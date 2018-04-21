#include "Base/Debug.h"
#include "Base/Utils.h"
#include "Base/RingBuffer.h"
#include "Net/TcpServer.h"
#include "Net/TcpClient.h"
#include "IPv4.h"

#pragma once

namespace TransProxy {

class TcpConnection;

typedef Utils::Map<Net::IPv4::SockAddrPair, TcpConnection> TcpConnections;

class TcpConnection: public Net::TcpConnection, public Utils::MapItem<
		Net::IPv4::SockAddrPair>, Utils::TimerListener {
	enum _State {
		STATE_CLOSED,
		STATE_LISTEN,
		STATE_SYN_SENT,
		STATE_SYN_RECV,
		STATE_ESTABLISHED,
		STATE_LAST_ACK,
		STATE_FIN_WAIT_1,
		STATE_FIN_WAIT_2,
		STATE_CLOSING,
		STATE_TIME_WAIT
	};

	TcpConnections* _conns;
	IPv4* _ipv4;
	Net::IPv4::SockAddrPair _addrs;
	Net::TcpServerListener* _serverListener;
	Net::TcpConnectionListener* _listener;
	uint16_t _id, _remoteWindowSize;
	_State _state;
	int _retryCount;
	uint32_t _localSeq, _remoteSeq;
	Utils::RingBuffer *_inRing, *_outRing;
	bool _needRecv, _needSend, _canRecv, _canSend, _needAck;

	struct _TimerListener2: Utils::TimerListener {
		TcpConnection* _this;
		_TimerListener2(TcpConnection* thiz) :
				_this(thiz) {
		}
		// Utils::TimerListener
		void onTimeout() THROWS;
		void onTimerError(Utils::Exception* e) THROWS {
			THROW(e);
		}
	} _timerListener2;

	Utils::Timer _timer1, _timer2;

	void sendPacket(int flags, size_t offset = 0, size_t bytes = 0,
			const void* data = NULL) THROWS;
	void _send(size_t offset, size_t bytes) THROWS;

	// Utils::TimerListener
	void onTimeout() THROWS;
	void onTimerError(Utils::Exception* e) THROWS {
		THROW(e);
	}

protected:
	virtual ~TcpConnection();

public:
	TcpConnection(TcpConnections* conns, IPv4* ipv4, uint32_t ip = 0);

	void dispatchPacket(Net::IPv4::TcpPacket& packet) THROWS;

	void accept(Net::IPv4::SockAddrPair addrs, Net::TcpServerListener* listener)
			THROWS;

	// Net::TcpConnection
	Net::TcpConnectionListener* setListener(
			Net::TcpConnectionListener* listener) {
		Net::TcpConnectionListener* prevListener = _listener;
		_listener = listener;
		return prevListener;
	}

	void connect(Net::IPv4::SockAddr addr) THROWS;

	Net::IPv4::SockAddr getLocalAddr() const {
		return _addrs.local;
	}
	Net::IPv4::SockAddr getRemoteAddr() const {
		return _addrs.remote;
	}

	void close() THROWS;

	void waitToRecv();
	void waitToSend();
	size_t peek(void* data, size_t bytes) THROWS;
	size_t recv(void* data, size_t bytes) THROWS;
	size_t send(const void* data, size_t bytes) THROWS;

	// MapItem
	Net::IPv4::SockAddrPair getKey() const {
		return _addrs;
	}
	Utils::String getKeyString() const {
		return _addrs.toString();
	}
};

}
