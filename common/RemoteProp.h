/**
\defgroup RemoteProp	Device Properties, Remote Properties
Simplifies handling of remote properties for Micromanager Devices
*/

/**
\ingroup	RemoteProp
\file		RemoteProp.h
\brief		Simplifies handling of remote properties for Micromanager Devices
\date		2016
\author		Jeffrey R. Kuhn <drjrkuhn@gmail.com>
\copyright	The University of Texas at Austin

$Id: RemoteProp.h 695 2017-05-20 18:51:47Z jkuhn $
$Author: jkuhn $
$Revision: 695 $
$Date: 2017-05-20 13:51:47 -0500 (Sat, 20 May 2017) $
*/

/**
\ingroup	RemoteProp
\page		AboutRemoteProp	About Remote Properties

These are classes to hold remote property values. 

They rely on the
HexProtocol architecture to pass properties between the host and 
the remote device.

Most of the marshalling between MicroManager Property changes and
contacting the remote device are handled by RemotePropBase.
Specifically, micromanager will call the RemotePropBase::OnExecute() 
method, which will in turn contact the device to send or receive
property values and sequences. Property values are cached as a local
copy.

RemotePropBase should not be used directly. Rather, one of its
sub-classes should be use. The following sub-classes define different
types of remote properties.

- **RemoteCachedProp** is used for properties for which there is only 
	a SET command. The getCachedValue holds the current property value.

- **RemoteReadOnlyProp** is used for properties for which there is
	only a GET command. The getCachedValue holds the last value retrieved
	from the device.

- **RemoteProp** is used for properties for which there is both a
	SET and a GET command. Although the getCachedValue is updated,
	Property updates always contact the device.

- **RemoteSequenceableProp** is used for a property which can be SET
	*and* can be *Sequenced* by MicroManager. The remote property
	will have a current value (stored in getCachedValue), but can
	also be set as an array. It is triggered by a startSeqCommand
	and ended with an endSeqCommand.

*/

#pragma once

#include "HexProtocol.h"
#include "DeviceHexProtocol.h"
#include "DeviceProp.h"
#include "DeviceError.h"
#include <regex>

namespace dprop {

	/** Throw an error during createRemotePropH on bad communication at creation? */
	const bool CREATE_FAILS_IF_ERR_COMMUNICATION = false;

	/////////////////////////////////////////////////////////////////////////////
	// CommandSet
	/////////////////////////////////////////////////////////////////////////////

	/** Helper Builder-pattern class for creating command sets.

	\ingroup RemoteProp

	For example,
	\code{.cpp}
	CommandSet cmds = CommandSet.build().withSet(SET_CMD).withGet(GET_CMD);
	prop.createRemoteProp(device, protocol, propInfo, cmds);
	\endcode

	*/
	class CommandSet {
	public:
		static CommandSet build() {
			return CommandSet();
		}

		virtual ~CommandSet() {}

		CommandSet& withSet(hprot::prot_cmd_t __cmd) {
			set_ = __cmd;
			return *this;
		}

		CommandSet& withGet(hprot::prot_cmd_t __cmd) {
			get_ = __cmd;
			return *this;
		}

		CommandSet& withSetSeq(hprot::prot_cmd_t __cmd) {
			setSeq_ = __cmd;
			return *this;
		}

		CommandSet& withGetSeq(hprot::prot_cmd_t __cmd) {
			getSeq_ = __cmd;
			return *this;
		}

		CommandSet& withStartSeq(hprot::prot_cmd_t __cmd) {
			startSeq_ = __cmd;
			return *this;
		}

		CommandSet& withStopSeq(hprot::prot_cmd_t __cmd) {
			stopSeq_ = __cmd;
			return *this;
		}

		CommandSet& withTask(hprot::prot_cmd_t __cmd) {
			task_ = __cmd;
			return *this;
		}

		CommandSet& withChan(hprot::prot_chan_t __chan) {
			chan_ = __chan;
			hasChan_ = true;
			return *this;
		}

		hprot::prot_cmd_t cmdGet() const {
			return get_;
		}

		hprot::prot_cmd_t cmdSet() const {
			return set_;
		}

		hprot::prot_cmd_t cmdSetSeq() const {
			return setSeq_;
		}

		hprot::prot_cmd_t cmdGetSeq() const {
			return getSeq_;
		}

		hprot::prot_cmd_t cmdStartSeq() const {
			return startSeq_;
		}

		hprot::prot_cmd_t cmdStopSeq() const {
			return stopSeq_;
		}

		hprot::prot_cmd_t cmdTask() const {
			return task_;
		}

		bool hasChan() const {
			return hasChan_;
		}

		hprot::prot_chan_t cmdChan() const {
			return chan_;
		}

	protected:
		template <typename T, class DEV, class HUB>
		friend class RemotePropBase;

		CommandSet() {}

		hprot::prot_cmd_t set_ = 0;
		hprot::prot_cmd_t get_ = 0;
		hprot::prot_cmd_t setSeq_ = 0;
		hprot::prot_cmd_t getSeq_ = 0;
		hprot::prot_cmd_t startSeq_ = 0;
		hprot::prot_cmd_t stopSeq_ = 0;
		hprot::prot_cmd_t task_ = 0;
		hprot::prot_chan_t chan_ = 0;
		bool hasChan_ = false;
	};

	/////////////////////////////////////////////////////////////////////////////
	// RemotePropBase
	/////////////////////////////////////////////////////////////////////////////

	/**
	A class to hold a remote property value.

	\ingroup RemoteProp

	Micromanager updates the
	property through the OnExecute member function, which in turn
	gets or sets the value from the device, or from a local cached value.
	Devices should not create a RemotePropBase directly, but should
	create one of its derived members and then call createRemoteProp.

	The T template parameter should contain the type of the member property.
	The type T should be able to auto-box and -unbox (auto-cast) from
	either MMInteger, MMFloat, or MMString.

	For example, if T=char, int, or long, then the property will be
	designated MM::Integer

	The DEV template parameter holds the device type
	@tparam T		property type
	@tparam DEV		device associated with property
	@tparam HUB		hub device, implements hprot::DeviceHexProtocol<HUB>
	*/
	template <typename T, class DEV, class HUB>
	class RemotePropBase : public DevicePropBase<T, DEV> {
		typedef DevicePropBase<T, DEV> BaseClass;
		typedef hprot::DeviceHexProtocol<HUB> ProtocolClass;
	protected:
		typedef MM::Action<RemotePropBase<T, DEV, HUB>> ActionType;

		RemotePropBase() : pProto_(nullptr) { }

		CommandSet cmds_;
		ProtocolClass* pProto_;

		virtual ~RemotePropBase() { }

		/**	Link the property to the __pDevice through the __pProtocol and initialize from the __propInfo.

			**THIS IS THE PRIMARY ENTRY POINT for creating remote properties.**
			*/
		int createRemotePropH(DEV* __pDevice, ProtocolClass* __pProtocol, const PropInfo<T>& __propInfo, CommandSet& __cmdSet) {
			pProto_ = __pProtocol;
			cmds_ = __cmdSet;
			int ret;
			bool readOnly = cmds_.cmdSet() == 0;
			bool useInitialValue = false;
			if (readOnly && cmds_.cmdGet()) {
				typename ProtocolClass::StreamGuard monitor(pProto_);
				// We do not have an initial value and must retrieve and cached the value from the device
				if ((ret = getRemoteValueH(BaseClass::cachedValue_)) != DEVICE_OK && CREATE_FAILS_IF_ERR_COMMUNICATION) {
					return ERR_COMMUNICATION;
				}
				useInitialValue = false;
			} else if (cmds_.cmdSet()) {
				useInitialValue = true;
			}
			MM::ActionFunctor* pAct = new ActionType(this, &RemotePropBase<T, DEV, HUB>::OnExecute);
			ret = createDevicePropH(__pDevice, __propInfo, pAct, readOnly, useInitialValue);
			if (ret == DEVICE_OK && cmds_.cmdSet()) {
				typename ProtocolClass::StreamGuard monitor(pProto_);
				// set the property on the remote device if possible
				if ((ret = setRemoteValueH(BaseClass::cachedValue_)) != DEVICE_OK && CREATE_FAILS_IF_ERR_COMMUNICATION) {
					return ERR_COMMUNICATION;
				}
			}
			return DEVICE_OK;
		}

		/* Helper function to retreive an array from the device.
		It is up the caller to pass the correct __getCmd. */
		template <typename E>
		std::vector<E> getRemoteArrayH(const hprot::prot_cmd_t __getCmd) {
			typename ProtocolClass::StreamGuard monitor(pProto_);
			hprot::prot_size_t size;
			if (cmds_.hasChan()) {
				if (!(__getCmd && pProto_->dispatchChannelGetArraySize(__getCmd, cmds_.cmdChan(), size))) {
#if LOG_REMOTE_ARRAYS != 0
					ProtocolClass::accessor::callLogMessage((HUB*)pProto_, "$$getRemoteArrayH-hasChan$$ Problem getting array size", false);
#endif
					return std::vector<E>();
				}
			} else {
				if (!(__getCmd && pProto_->dispatchGetArraySize(__getCmd, size))) {
#if LOG_REMOTE_ARRAYS != 0
					ProtocolClass::accessor::callLogMessage((HUB*)pProto_, "$$getRemoteArrayH$$ Problem getting array size", false);
#endif
					return std::vector<E>();
				}
			}
			// Preallocate the array elements and convert to an array pointer using .data() for filling by dispatchGetArray()
			std::vector<E> array(size);
			size = static_cast<hprot::prot_size_t>(array.size());
			std::ostringstream msg;
			msg << "$$getRemoteArrayH$$ Got array of size " << size;
			if (cmds_.hasChan()) {
				if (pProto_->dispatchChannelGetArray(__getCmd, cmds_.cmdChan(), array.data(), size, size)) {
#if LOG_REMOTE_ARRAYS != 0
					msg << " chan " << cmds_.cmdChan() << " : ";
					std::copy(array.begin(), array.end(), std::ostream_iterator<E>(msg, "; "));
					ProtocolClass::accessor::callLogMessage((HUB*)pProto_, msg.str().c_str(), false);
#endif
					return array;
				}
			} else {
				if (pProto_->dispatchGetArray(__getCmd, array.data(), size, size)) {
#if LOG_REMOTE_ARRAYS != 0
					msg << " : ";
					std::copy(array.begin(), array.end(), std::ostream_iterator<E>(msg, "; "));
					ProtocolClass::accessor::callLogMessage((HUB*)pProto_, msg.str().c_str(), false);
#endif
					return array;
				}
			}
#if LOG_REMOTE_ARRAYS != 0
			ProtocolClass::accessor::callLogMessage((HUB*)pProto_, "$$getRemoteArrayH$$ empty array", false);
#endif
			return std::vector<E>();
		}

		/* Helper function to retrieve the maximum settable size of a remote array.
		It is up the caller to pass the correct __setCmd. */
		template <typename E>
		hprot::prot_size_t getRemoteArrayMaxSizeH(const hprot::prot_cmd_t __setCmd) const {
			hprot::prot_size_t maxSize;
			if (cmds_.hasChan()) {
				if (!(__setCmd && pProto_->dispatchChannelGetArrayMaxSize(__setCmd, cmds_.cmdChan(), maxSize))) {
#if LOG_REMOTE_ARRAYS != 0
					ProtocolClass::accessor::callLogMessage((HUB*)pProto_, "getRemoteArrayMaxSizeH-hasChan$$ Problem getting array size", false);
#endif
					return 0;
				}
			} else {
				if (!(__setCmd && pProto_->dispatchGetArrayMaxSize(__setCmd, maxSize))) {
#if LOG_REMOTE_ARRAYS != 0
					ProtocolClass::accessor::callLogMessage((HUB*)pProto_, "getRemoteArrayMaxSizeH$$ Problem getting array size", false);
#endif
					return 0;
				}
			}
#if LOG_REMOTE_ARRAYS != 0
			std::ostringstream msg;
			msg << "$$getRemoteArrayMaxSizeH$$ Got array of size " << maxSize;
			ProtocolClass::accessor::callLogMessage((HUB*)pProto_, msg.str().c_str(), false);
#endif
			return maxSize;
		}

		/* Helper function put put an array on the remote device.
		It is up the caller to pass the correct __setCmd. */
		template <typename E>
		bool putRemoteArrayH(const hprot::prot_cmd_t __setCmd, const std::vector<E> __array, hprot::prot_size_t __remoteMaxSeqSize) {
			typename ProtocolClass::StreamGuard monitor(pProto_);
			// NOTE: the __remoteMaxSeqSize argument is mainly there to insure that we have
			// already called getRemoteMaxSeqSize().
			hprot::prot_size_t size = static_cast<hprot::prot_size_t>(__array.size());
			if (size > __remoteMaxSeqSize) {
				return false;
			}
			// Send the values
			if (cmds_.hasChan()) {
				return __setCmd && pProto_->dispatchChannelSetArray(__setCmd, cmds_.cmdChan() , __array.data(), size);
			} else {
				return __setCmd && pProto_->dispatchSetArray(__setCmd, __array.data(), size);
			}
		}

		/* Helper function to covert an array of strings and put it on the device.
		It is up the caller to pass the correct __setCmd. The caller
		mast have alread used getRemoteArrayMaxSizeH to get the maximum size and
		placed the value in __remoteMaxSeqSize. */
		template <typename E>
		bool putRemoteStringArrayH(const hprot::prot_cmd_t __setCmd, const std::vector<std::string> __strArray, hprot::prot_size_t __remoteMaxSeqSize) {
			hprot::prot_size_t size = static_cast<hprot::prot_size_t>(__strArray.size());
			if (size > __remoteMaxSeqSize) {
				return false;
			}
			// Copy the string array to an array of values
			std::vector<E> valueArray;
			for (auto s : __strArray) {
				E val;
				ParseValue<E>(val, s);
				valueArray.push_back(val);
			}
			return putRemoteArrayH<E>(__setCmd, valueArray, __remoteMaxSeqSize);
		}

		////////////////////////////////////////////////////////////////////
		/// Property getting/setting.
		/// Sub-classes may override to change the default behavior

		/** Get the value from the remote. Derived classes may override. */
		virtual int getRemoteValueH(T& __val) {
			if (cmds_.hasChan()) {
				if (pProto_->dispatchChannelGet(cmds_.cmdGet(), cmds_.cmdChan(), __val)) {
					return DEVICE_OK;
				}
			} else {
				if (pProto_->dispatchGet(cmds_.cmdGet(), __val)) {
					return DEVICE_OK;
				}
			}
			return ERR_COMMUNICATION;
		}

		/** Set the value on the remote. Derived classes may override. */
		virtual int setRemoteValueH(const T& __val) {
			if (cmds_.hasChan()) {
				if (pProto_->dispatchChannelSet(cmds_.cmdSet(), cmds_.cmdChan(), __val)) {
					return DEVICE_OK;
				}
			} else {
				if (pProto_->dispatchSet(cmds_.cmdSet(), __val)) {
					return DEVICE_OK;
				}
			}
			return ERR_COMMUNICATION;
		}

		////////////////////////////////////////////////////////////////////
		/// Sequence setting and triggering
		/// Sub-classes may override to change the default behavior

		/** Get the maximum size of the remote sequence. Derived classes may override. */
		virtual int getRemoteSequenceSizeH(hprot::prot_size_t& __size) const {
			__size = getRemoteArrayMaxSizeH<T>(cmds_.cmdSetSeq());
			return DEVICE_OK;
		}

		/** Set a remote sequence. Derived classes may override. */
		virtual int setRemoteSequenceH(const std::vector<std::string> __sequence) {
			hprot::prot_size_t maxSize = getRemoteArrayMaxSizeH<T>(cmds_.cmdSetSeq());
			if (__sequence.size() > maxSize) {
				return DEVICE_SEQUENCE_TOO_LARGE;
			}
			// Use a helper function to send the sequence string array
			// to the device
			if (!putRemoteStringArrayH<T>(cmds_.cmdSetSeq(), __sequence, maxSize)) {
				return ERR_COMMUNICATION;
			}
			return DEVICE_OK;
		}

		/** Start the remote sequence. Derived classes may override. */
		virtual int startRemoteSequenceH() {
			if (cmds_.hasChan()) {
				if (pProto_->dispatchChannelTask(cmds_.cmdStartSeq(), cmds_.cmdChan())) {
					return DEVICE_OK;
				}
			} else {
				if (pProto_->dispatchTask(cmds_.cmdStartSeq())) {
					return DEVICE_OK;
				}
			}
			return ERR_COMMUNICATION;
		}

		/** Stop the remote sequence. Derived classes may override. */
		virtual int stopRemoteSequenceH() {
			if (cmds_.hasChan()) {
				if (pProto_->dispatchChannelTask(cmds_.cmdStopSeq(), cmds_.cmdChan())) {
					return DEVICE_OK;
				}
			} else {
				if (pProto_->dispatchTask(cmds_.cmdStopSeq())) {
					return DEVICE_OK;
				}
			}
			return ERR_COMMUNICATION;
		}

		/* Called by the properties update method.
			 This is the main Property update routine. */
		virtual int OnExecute(MM::PropertyBase* pProp, MM::ActionType eAct) override {
			typename ProtocolClass::StreamGuard monitor(pProto_);
			int result;
			if (eAct == MM::BeforeGet) {
				if (cmds_.cmdGet()) {
					// read the value from the remote device
					T temp;
					if ((result = getRemoteValueH(temp)) != DEVICE_OK) {
						return result;
					}
					BaseClass::cachedValue_ = temp;
					SetProp<T>(pProp, BaseClass::cachedValue_);
				} else {
					// Just use the getCachedValue
					SetProp<T>(pProp, BaseClass::cachedValue_);
				}
			} else if (cmds_.cmdSet() && eAct == MM::AfterSet) {
				T temp;
				SetValue<T>(temp, pProp);
				if ((result = setRemoteValueH(temp)) != DEVICE_OK) {
					return result;
				}
				BaseClass::cachedValue_ = temp;
				return notifyChangeH(BaseClass::cachedValue_);
			} else if (cmds_.cmdSetSeq() && eAct == MM::IsSequenceable) {
				hprot::prot_size_t maxSize;
				if ((result = getRemoteSequenceSizeH(maxSize)) != DEVICE_OK) {
					return result;
				}
				// maxSize will be zero if there was no setSeqCommand 
				// or an error occurred. SetSequencable(0) indicates
				// that the property cannot be sequenced
				pProp->SetSequenceable(maxSize);
			} else if (cmds_.cmdSetSeq() && eAct == MM::AfterLoadSequence) {
				std::vector<std::string> sequence = pProp->GetSequence();
				if ((result = setRemoteSequenceH(sequence)) != DEVICE_OK) {
					return result;
				}
			} else if (cmds_.cmdSetSeq() && eAct == MM::StartSequence) {
				if ((result = startRemoteSequenceH()) != DEVICE_OK) {
					return result;
				}
			} else if (cmds_.cmdSetSeq() && eAct == MM::StopSequence) {
				if ((result = stopRemoteSequenceH()) != DEVICE_OK) {
					return result;
				}
			}
			return DEVICE_OK;
		}
	};

	/////////////////////////////////////////////////////////////////////////////
	// Specific RemoteProp implementations
	/////////////////////////////////////////////////////////////////////////////

	/**
		A class to hold a read/write remote property value.

		\ingroup RemoteProp

		Micromanager updates the
		property through the OnExecute member function, which in turn
		gets or sets the value from the device.
	*/
	template <typename T, class DEV, class HUB>
	class RemoteProp : public RemotePropBase<T, DEV, HUB> {
		typedef hprot::DeviceHexProtocol<HUB> ProtocolClass;
	public:
		int createRemoteProp(DEV* __pDevice, ProtocolClass* __pProtocol, const PropInfo<T>& __propInfo, CommandSet& __cmds) {
			assert(__cmds.cmdSet() || __cmds.cmdGet());
			return createRemotePropH(__pDevice, __pProtocol, __propInfo, __cmds);
		}
	};


	/**
	A class to hold a write-only remote property value.

	\ingroup RemoteProp

	Micromanager updates
	the property through the OnExecute member function, which in turn
	sets the value from the device and updates the getCachedValue.
	Get simply returns the last cached value.
	*/
	template <typename T, class DEV, class HUB>
	class RemoteCachedProp : public RemotePropBase<T, DEV, HUB> {
		typedef hprot::DeviceHexProtocol<HUB> ProtocolClass;
	public:
		int createRemoteProp(DEV* __pDevice, ProtocolClass* __pProtocol, const PropInfo<T>& __propInfo, CommandSet& __cmds) {
			assert(__cmds.cmdSet());
			return createRemotePropH(__pDevice, __pProtocol, __propInfo, __cmds);
		}
	};

	/**
	A class to hold a sequencable write-only remote property value.

	\ingroup RemoteProp

	Micromanager updates the property through the OnExecute member function,
	which in turn sets the value from the device and updates the getCachedValue.
	Get simply returns the last cached value. This version exposes the sequencable
	helper interface.

	*/
	template <typename T, class DEV, class HUB>
	class RemoteSequenceableProp : public RemotePropBase<T, DEV, HUB> {
		typedef hprot::DeviceHexProtocol<HUB> ProtocolClass;
	public:
		int createRemoteProp(DEV* __pDevice, ProtocolClass* __pProtocol, const PropInfo<T>& __propInfo, CommandSet& __cmds) {
			assert(__cmds.cmdSet() && __cmds.cmdSetSeq() && __cmds.cmdStartSeq() && __cmds.cmdStopSeq());
			return createRemotePropH(__pDevice, __pProtocol, __propInfo, __cmds);
		}

		std::vector<T> GetRemoteArray() {
			return getRemoteArrayH<T>(RemotePropBase<T, DEV, HUB>::cmds_.cmdGetSeq());
		}

		/** Get the maximum size of the remote sequence. */
		int getRemoteSequenceSize(hprot::prot_size_t& __size) const {
			return getRemoteSequenceSizeH(__size);
		}

		/** Set a remote sequence. */
		int setRemoteSequence(const std::vector<std::string> __sequence) {
			return setRemoteSequenceH(__sequence);
		}

		/** Start the remote sequence. */
		int startRemoteSequence() {
			return startRemoteSequenceH();
		}

		/** Stop the remote sequence. */
		int stopRemoteSequence() {
			return stopRemoteSequenceH();
		}


	protected:
		
	};

	/**
	A class to hold a read-only remote property value.

	\ingroup RemoteProp

	Micromanager updates
	the property through the OnExecute member function, which in turn
	gets the value from the device and updates the getCachedValue.
	Sets do nothing.
	*/
	template <typename T, class DEV, class HUB>
	class RemoteReadOnlyProp : public RemotePropBase<T, DEV, HUB> {
		typedef RemotePropBase<T, DEV, HUB> BaseClass;
		typedef hprot::DeviceHexProtocol<HUB> ProtocolClass;
	public:
		int createRemoteProp(DEV* __pDevice, ProtocolClass* __pProtocol, const PropInfo<T>& __propInfo, CommandSet& __cmds) {
			assert(__cmds.cmdGet());
			return createRemotePropH(__pDevice, __pProtocol, __propInfo, __cmds);
		}
	};

	/**
	A class to hold a read/write remote array property value as strings.

	\ingroup RemoteProp

	Micromanager properties cannot be arrays. This is a work-around that
	represents an array as a Micromanager StringProperty. This class
	marshals between arrays on the remote device and a string containing
	a textual representation of the array. A java program may read/write
	the string to decode or encode the array.

	Micromanager updates the property through the OnExecute member function,
	which in turn gets or sets the value from the device.

	template parameter E is the type of each array element.
	The RemotePropBase is std::string, because that is Micromanager's
	view of the Property value.

	## Array Format

	####Converting strings to arrays

	- Array values are separated by a semicolon ';' character.
	- Spaces before and after the semicolon are ignored.
	- Trailing semicolons are ignored.
	- Two successive semicolons ("; ;") are considered to have a value.
	+ For integers and floats, the value is zero.
	+ for strings, the value is an empty string.
	- For an array of strings, the string cannot have a semicolon,
	as this would be treated as a token separator

	type	| String			| array result
	----	| --------			| ----------
	int		| "1 ; 2; 3; 4;"	| {1, 2, 3, 4}
	int		| "1 ; 2; 3; 4"		| {1, 2, 3, 4}
	int		| "1 ; ;; 4"		| {1, 0, 0, 4}
	string	| "hello world; foo; bar ;;"	| {"hello world", "foo", "bar", ""}

	####Converting arrays to strings

	- Arrays elements are converted in order and separated by a
	semicolon and space ("; ").
	- The final string will *not* have a trailing semicolon *unless*
	the element type is a string and the last string was empty.

	type	| array								| string result
	----	| --------------------------------	| ----------
	int		| {1, 2, 3, 4}						| "1; 2; 3; 4"
	string	| {"aaa", "bbb", "ccc"}				| "aaa; bbb; ccc"
	string	| {"hello world", "foo", "bar", ""}	| "hello world; foo; bar; ;"

	*/
	template <typename E, class DEV, class HUB>
	class RemoteArrayProp : public RemotePropBase<std::string, DEV, HUB> {
		typedef RemotePropBase<std::string, DEV, HUB> BaseClass;
		typedef hprot::DeviceHexProtocol<HUB> ProtocolClass;
	public:

		/** Creates a remote property for a device. */
		int createRemoteProp(DEV* __pDevice, ProtocolClass* __pProtocol, const PropInfo<std::string>& __propInfo, CommandSet& __cmds) {
			assert(__cmds.cmdSet() || __cmds.cmdGet());
			return createRemotePropH(__pDevice, __pProtocol, __propInfo, __cmds);
		}

		/** Set the input and output separator strings. The  __inputSep is a regular expression
		that is used to search for the token separator. The __outputSep is a simple string
		that is added between each pair of array elements on the output. The __outputSep
		is not added after the last element.
		@param __inputSep the input separator token regular expression. 
					Defaults to <tt>std::regex("\\s*;\\s*")</tt>. 
		@param __outputSep the output separator string. 
					Defaults to <tt>"; "</tt>.
		*/
		void separators(std::regex __inputSep, const char* __outputSep) {
			inSep_ = __inputSep;
			outSep_ = __outputSep;
		}

	protected:
		std::regex inSep_ = std::regex("\\s*;\\s*");
		const char* outSep_ = "; ";

		/** Convert a string to a vector of elements. */
		std::vector<E> marshalStringH(const std::string __arrStr) {
			std::vector<E> res;
			if (__arrStr.empty()) {
				return res;
			}
			std::sregex_token_iterator it_next(__arrStr.begin(), __arrStr.end(), inSep_, -1);
			std::sregex_token_iterator it_end;
			for (; it_next != it_end; ++it_next) {
				E el;
				ParseValue<E>(el, it_next->str());
				res.push_back(el);
			}
			return res;
		}

		/** Convert a vector of elements to a string. */
		std::string marshalArrayH(const std::vector<E> __arr) {
			if (__arr.size() == 0) {
				return "";
			}
			std::stringstream os;
			auto last = __arr.end();
			--last;
			for (auto it = __arr.begin(); it != __arr.end(); ++it) {
				os << MarshalValue<E>(*it);
				if (it != last) {
					os << outSep_;
				}
			}
			return os.str();
		}

		/** Get the array value from the remote and turn it into a string. Overrides RemoteProtBase definition. */
		int getRemoteValueH(std::string& __prop) override {
			std::vector<E> arr = BaseClass::template getRemoteArrayH<E>(BaseClass::cmds_.cmdGet());
			__prop = marshalArrayH(arr);
			return DEVICE_OK;
		}

		/** Convert the property value to an array and set the value on the remote. Overrides RemoteProtBase definition. */
		int setRemoteValueH(const std::string& __prop) override {
			hprot::prot_size_t maxSize = BaseClass::template getRemoteArrayMaxSizeH<E>(BaseClass::cmds_.cmdSet());
			std::vector<E> arr = marshalStringH(__prop);
			if (arr.size() > maxSize) {
				return DEVICE_SEQUENCE_TOO_LARGE;
			}
			// Use a helper function to send the sequence string array
			// to the device
			if (!putRemoteArrayH<E>(BaseClass::cmds_.cmdSet(), arr, maxSize)) {
				return ERR_COMMUNICATION;
			}
			return DEVICE_OK;
		}
	};

}; // namespace dprop


