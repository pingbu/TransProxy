#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdlib.h>
#include "Base/Math.h"
#include "Base/String.h"
#include "Base/Debug.h"
#include "Packet.h"

#pragma once

namespace Net {

namespace IPv4 {

static inline uint32_t aton(const char* ip) {
	return ntohl(::inet_addr(ip));
}

static inline const char* ntoa(uint32_t ip) {
	struct in_addr ia;
	ia.s_addr = htonl(ip);
	return ::inet_ntoa(ia);
}

static inline bool isLanIP(uint32_t ip) {
	return (ip & 0xFF000000) == 0x0A000000 // 10.0.0.0 mask 255.0.0.0
	|| (ip & 0xFFC00000) == 0x64400000 // 100.64.0.0 mask 255.192.0.0
	|| (ip & 0xFFF00000) == 0xAC100000 // 172.16.0.0 mask 255.240.0.0
	|| (ip & 0xFFFF0000) == 0xC0A80000; // 192.168.0.0 mask 255.255.0.0
}

struct SockAddr {
	uint32_t ip;
	uint16_t port;
	SockAddr() :
			ip(0), port(0) {
	}
	SockAddr(uint32_t ip, uint16_t port) :
			ip(ip), port(port) {
	}
	SockAddr(const char* hostname, uint16_t port) :
			ip(aton(hostname)), port(port) {
	}
	SockAddr(const sockaddr_in& addr) :
			ip(ntohl(addr.sin_addr.s_addr)), port(ntohs(addr.sin_port)) {
	}
	SockAddr(const char* addr) THROWS {
		const char* p = ::strchr(addr, ':');
		THROW_IF(p == addr,
				new Utils::Exception("Invalid socket address '%s'", addr));
		if (p == NULL) {
			this->ip = aton(addr);
			this->port = 0;
		} else {
			int port = ::atoi(p + 1);
			THROW_IF(port <= 0 || port >= 65536,
					new Utils::Exception("Invalid port in socket address '%s'",
							addr));
			this->ip = aton(Utils::String(addr, p - addr).sz());
			this->port = (uint16_t) port;
		}
	}
	Utils::String toString() const {
		return Utils::String::format("%s:%u", ntoa(ip), port);
	}
	bool operator==(const SockAddr& addr) const {
		return ip == addr.ip && port == addr.port;
	}
	bool operator!=(const SockAddr& addr) const {
		return ip != addr.ip || port != addr.port;
	}
	bool operator<(const SockAddr& addr) const {
		return ip < addr.ip || (ip == addr.ip && port < addr.port);
	}
	bool operator<=(const SockAddr& addr) const {
		return ip < addr.ip || (ip == addr.ip && port <= addr.port);
	}
	bool operator>(const SockAddr& addr) const {
		return ip > addr.ip || (ip == addr.ip && port > addr.port);
	}
	bool operator>=(const SockAddr& addr) const {
		return ip > addr.ip || (ip == addr.ip && port >= addr.port);
	}
};

struct SockAddrPair {
	SockAddr remote, local;
	SockAddrPair() {
	}
	SockAddrPair(SockAddr remote, SockAddr local) :
			remote(remote), local(local) {
	}
	Utils::String toString() const {
		Utils::String remoteIP = ntoa(remote.ip);
		return Utils::String::format("%s:%u->%s:%u", remoteIP.sz(), remote.port,
				ntoa(local.ip), local.port);
	}
	bool operator==(const SockAddrPair& pair) const {
		return local == pair.local && remote == pair.remote;
	}
	bool operator!=(const SockAddrPair& pair) const {
		return local != pair.local || remote != pair.remote;
	}
	bool operator<(const SockAddrPair& pair) const {
		return local < pair.local
				|| (local == pair.local && remote < pair.remote);
	}
	bool operator<=(const SockAddrPair& pair) const {
		return local < pair.local
				|| (local == pair.local && remote <= pair.remote);
	}
	bool operator>(const SockAddrPair& pair) const {
		return local > pair.local
				|| (local == pair.local && remote > pair.remote);
	}
	bool operator>=(const SockAddrPair& pair) const {
		return local > pair.local
				|| (local == pair.local && remote >= pair.remote);
	}
};

struct ServiceAddr {
	int proto;
	SockAddr sockAddr;
	ServiceAddr() :
			proto(PROTO_NULL) {
	}
	ServiceAddr(int proto, SockAddr sockAddr) :
			proto(proto), sockAddr(sockAddr) {
	}
	ServiceAddr(int proto, uint32_t ip, uint16_t port) :
			proto(proto), sockAddr(ip, port) {
	}
	ServiceAddr(const char* url) THROWS {
		if (::strncmp(url, "tcp://", 6) == 0) {
			proto = PROTO_TCP;
			sockAddr = url + 6;
		} else if (::strncmp(url, "udp://", 6) == 0) {
			proto = PROTO_UDP;
			sockAddr = url + 6;
		} else {
			THROW(new Utils::Exception("Invalid service address '%s'", url));
			proto = PROTO_NULL;
		}
	}
	Utils::String toString() const {
		return Utils::String::format(
				sockAddr.port == 0 ? "%s://%s" : "%s://%s:%u",
				proto == PROTO_TCP ? "tcp" : proto == PROTO_UDP ? "udp" : "?",
				ntoa(sockAddr.ip), sockAddr.port);
	}
	bool operator==(const ServiceAddr& addr) const {
		return proto == addr.proto && sockAddr == addr.sockAddr;
	}
	bool operator!=(const ServiceAddr& addr) const {
		return proto != addr.proto || sockAddr != addr.sockAddr;
	}
	bool operator<(const ServiceAddr& addr) const {
		return proto < addr.proto
				|| (proto == addr.proto && sockAddr < addr.sockAddr);
	}
	bool operator<=(const ServiceAddr& addr) const {
		return proto < addr.proto
				|| (proto == addr.proto && sockAddr <= addr.sockAddr);
	}
	bool operator>(const ServiceAddr& addr) const {
		return proto > addr.proto
				|| (proto == addr.proto && sockAddr > addr.sockAddr);
	}
	bool operator>=(const ServiceAddr& addr) const {
		return proto > addr.proto
				|| (proto == addr.proto && sockAddr >= addr.sockAddr);
	}
};

class IpPacket: public Packet {
	size_t _hdrlen;
protected:
	IpPacket(void* ptr, size_t size, uint8_t protocol) :
			Packet(ptr, size) {
		_hdrlen = 20;
		uint8_t* header = (uint8_t*) ptr;
		header[0] = (uint8_t) (0x40 | (_hdrlen / 4));
		header[1] = 0;
		Packet::write16(6, 0); // Slice Info
		header[8] = 128; // TTL
		header[9] = protocol;
		setDataSize(0);
	}
	size_t headerSize() const {
		return _hdrlen;
	}
public:
	static bool isValid(void* ptr) {
		return (*(uint8_t*) ptr >> 4) == 4;
	}
	static uint8_t getProtocol(void* ptr) {
		return ((uint8_t*) ptr)[9];
	}
	IpPacket(void* ptr, size_t size) :
			Packet(ptr, size) {
		THROW_IF(!isValid(ptr), new Utils::Exception("Not IPv4 packet"));
		_hdrlen = ((*(Packet*) this)[0] & 0x0F) * 4;
	}
	virtual ~IpPacket() {
	}
	size_t packetSize() const {
		return Packet::read16(2);
	}
	uint8_t* dataPtr() {
		return ptr() + _hdrlen;
	}
	const uint8_t* dataPtr() const {
		return ptr() + _hdrlen;
	}
	size_t getDataSize() const {
		return packetSize() - _hdrlen;
	}
	void setDataSize(uint16_t size) {
		Packet::write16(2, _hdrlen + size);
	}
	uint16_t getId() const {
		return Packet::read16(4);
	}
	void setId(uint32_t id) {
		Packet::write16(4, id);
	}
	uint8_t protocol() const {
		return (*(Packet*) this)[9];
	}
	uint32_t getSrcAddr() const {
		return Packet::read32(12);
	}
	void setSrcAddr(uint32_t ip) {
		Packet::write32(12, ip);
	}
	uint32_t getDestAddr() const {
		return Packet::read32(16);
	}
	void setDestAddr(uint32_t ip) {
		Packet::write32(16, ip);
	}
	uint8_t& operator[](size_t index) {
		return (*(Packet*) this)[_hdrlen + index];
	}
	uint8_t operator[](size_t index) const {
		return (*(Packet*) this)[_hdrlen + index];
	}
	uint16_t read16(size_t offset) const {
		return Packet::read16(_hdrlen + offset);
	}
	uint32_t read32(size_t offset) const {
		return Packet::read32(_hdrlen + offset);
	}
	void read(size_t offset, void* data, size_t bytes) const {
		Packet::read(_hdrlen + offset, data, bytes);
	}
	void write16(size_t offset, uint16_t v) {
		Packet::write16(_hdrlen + offset, v);
	}
	void write32(size_t offset, uint32_t v) {
		Packet::write32(_hdrlen + offset, v);
	}
	void write(size_t offset, const void* data, size_t bytes) {
		Packet::write(_hdrlen + offset, data, bytes);
	}
	void fillChecksum() {
		uint8_t* header = ptr();
		uint16_t& checksum = *(uint16_t*) (header + 10);
		checksum = 0;
		checksum = Utils::checksum(Utils::sum(header, _hdrlen));
		//Log::d("IP checksum = 0x%04X", checksum);
	}
};

class TcpPacket: public IpPacket {
	size_t _hdrlen;
	void _init() THROWS {
		THROW_IF(IpPacket::protocol() != IPPROTO_TCP,
				new Utils::Exception("Not TCP packet"));
		_hdrlen = (IpPacket::dataPtr()[12] >> 4) * 4;
	}
protected:
	TcpPacket(void* ptr) :
			IpPacket(ptr, 1500, IPPROTO_TCP) {
		_hdrlen = 20;
		IpPacket::dataPtr()[12] = (_hdrlen / 4) << 4;
		IpPacket::dataPtr()[13] = 0;
		setWindowSize(0);
		IpPacket::write16(18, 0); // Urgent Pointer
		setDataSize(0);
	}
public:
	enum {
		FLAG_FIN = 1,
		FLAG_SYN = 2,
		FLAG_RST = 4,
		FLAG_PSH = 8,
		FLAG_ACK = 16,
		FLAG_URG = 32
	};
	static bool isValid(void* ptr) {
		return IpPacket::isValid(ptr)
				&& IpPacket::getProtocol(ptr) == IPPROTO_TCP;
	}
	static bool isValid(const IpPacket& packet) {
		return packet.protocol() == IPPROTO_TCP;
	}
	TcpPacket(void* ptr, size_t size) :
			IpPacket(ptr, size) THROWS {
		_init();
	}
	TcpPacket(IpPacket& packet) :
			IpPacket(packet) THROWS {
		_init();
	}
	virtual ~TcpPacket() {
	}
	Utils::String toString() const {
		Utils::String flags = "";
		if (hasFlags(FLAG_FIN))
			flags += "+FIN";
		if (hasFlags(FLAG_SYN))
			flags += "+SYN";
		if (hasFlags(FLAG_RST))
			flags += "+RST";
		if (hasFlags(FLAG_PSH))
			flags += "+PSH";
		if (hasFlags(FLAG_ACK))
			flags += "+ACK";
		if (hasFlags(FLAG_URG))
			flags += "+URG";
		if (flags.length() > 0)
			flags = flags.substring(1);
		return Utils::String::format("%s, seq=%u, ack=%u, data=%u, win=%u",
				flags.sz(), getSeq(), getAck(), getDataSize(), getWindowSize());
	}
	uint8_t* dataPtr() {
		return IpPacket::dataPtr() + _hdrlen;
	}
	const uint8_t* dataPtr() const {
		return IpPacket::dataPtr() + _hdrlen;
	}
	size_t getDataSize() const {
		return IpPacket::getDataSize() - _hdrlen;
	}
	void setDataSize(size_t size) {
		IpPacket::setDataSize(_hdrlen + size);
	}
	uint16_t getSrcPort() const {
		return IpPacket::read16(0);
	}
	void setSrcPort(uint16_t port) {
		IpPacket::write16(0, port);
	}
	uint16_t getDestPort() const {
		return IpPacket::read16(2);
	}
	void setDestPort(uint16_t port) {
		IpPacket::write16(2, port);
	}
	SockAddr getSrcSockAddr() const {
		return SockAddr(getSrcAddr(), getSrcPort());
	}
	void setSrcSockAddr(SockAddr addr) {
		setSrcAddr(addr.ip);
		setSrcPort(addr.port);
	}
	SockAddr getDestSockAddr() const {
		return SockAddr(getDestAddr(), getDestPort());
	}
	void setDestSockAddr(SockAddr addr) {
		setDestAddr(addr.ip);
		setDestPort(addr.port);
	}
	uint32_t getSeq() const {
		return IpPacket::read32(4);
	}
	void setSeq(uint32_t seq) {
		IpPacket::write32(4, seq);
	}
	uint32_t getAck() const {
		return IpPacket::read32(8);
	}
	void setAck(uint32_t ack) {
		IpPacket::write32(8, ack);
	}
	bool hasFlags(int flags) const {
		return (IpPacket::dataPtr()[13] & flags) == flags;
	}
	void addFlags(int flags) {
		IpPacket::dataPtr()[13] |= flags;
	}
	void setFlags(int flags) {
		IpPacket::dataPtr()[13] = flags;
	}
	uint16_t getWindowSize() const {
		return IpPacket::read16(14);
	}
	void setWindowSize(uint16_t bytes) {
		IpPacket::write16(14, bytes);
	}
	uint8_t& operator[](size_t index) {
		return (*(IpPacket*) this)[_hdrlen + index];
	}
	uint8_t operator[](size_t index) const {
		return (*(IpPacket*) this)[_hdrlen + index];
	}
	uint16_t read16(size_t offset) const {
		return IpPacket::read16(_hdrlen + offset);
	}
	uint32_t read32(size_t offset) const {
		return IpPacket::read32(_hdrlen + offset);
	}
	void read(size_t offset, void* data, size_t bytes) const {
		IpPacket::read(_hdrlen + offset, data, bytes);
	}
	void write16(size_t offset, uint16_t v) {
		IpPacket::write16(_hdrlen + offset, v);
	}
	void write32(size_t offset, uint32_t v) {
		IpPacket::write32(_hdrlen + offset, v);
	}
	void write(size_t offset, const void* data, size_t bytes) {
		IpPacket::write(_hdrlen + offset, data, bytes);
	}
	void fillChecksum() {
		IpPacket::fillChecksum();

		uint8_t* header = ptr();
		uint8_t* data = header + IpPacket::headerSize();
		size_t datalen = IpPacket::getDataSize();

		uint16_t& checksum = *(uint16_t*) (data + 16);
		checksum = 0;
		checksum = Utils::checksum(
				Utils::sum(header + 12, 8) + IPPROTO_TCP + datalen
						+ Utils::sum(data, datalen));
		//Log::d("TCP checksum = 0x%04X", checksum);
	}
};

class TcpPacketBuffer: public TcpPacket {
	uint8_t _buf[1500];
public:
	TcpPacketBuffer() :
			TcpPacket(_buf) {
	}
};

class UdpPacket: public IpPacket {
protected:
	UdpPacket(void* ptr) :
			IpPacket(ptr, 1500, IPPROTO_UDP) {
		setDataSize(0);
	}
public:
	static bool isValid(void* ptr) {
		return IpPacket::isValid(ptr)
				&& IpPacket::getProtocol(ptr) == IPPROTO_UDP;
	}
	static bool isValid(const IpPacket& packet) {
		return packet.protocol() == IPPROTO_UDP;
	}
	UdpPacket(void* ptr, size_t size) :
			IpPacket(ptr, size) THROWS {
		THROW_IF(IpPacket::protocol() != IPPROTO_UDP,
				new Utils::Exception("Not UDP packet"));
	}
	UdpPacket(IpPacket& packet) :
			IpPacket(packet) THROWS {
		THROW_IF(IpPacket::protocol() != IPPROTO_UDP,
				new Utils::Exception("Not UDP packet"));
	}
	virtual ~UdpPacket() {
	}
	uint8_t* dataPtr() {
		return IpPacket::dataPtr() + 8;
	}
	const uint8_t* dataPtr() const {
		return IpPacket::dataPtr() + 8;
	}
	size_t getDataSize() const {
		return IpPacket::read16(4) - 8;
	}
	void setDataSize(size_t size) {
		size += 8;
		IpPacket::write16(4, (uint16_t) size);
		IpPacket::setDataSize(size);
	}
	uint16_t getSrcPort() const {
		return IpPacket::read16(0);
	}
	void setSrcPort(uint16_t port) {
		IpPacket::write16(0, port);
	}
	uint16_t getDestPort() const {
		return IpPacket::read16(2);
	}
	void setDestPort(uint16_t port) {
		IpPacket::write16(2, port);
	}
	uint8_t& operator[](size_t index) {
		return (*(IpPacket*) this)[8 + index];
	}
	uint8_t operator[](size_t index) const {
		return (*(IpPacket*) this)[8 + index];
	}
	uint16_t read16(size_t offset) const {
		return IpPacket::read16(8 + offset);
	}
	uint32_t read32(size_t offset) const {
		return IpPacket::read32(8 + offset);
	}
	void read(size_t offset, void* data, size_t bytes) const {
		IpPacket::read(8 + offset, data, bytes);
	}
	void write16(size_t offset, uint16_t v) {
		IpPacket::write16(8 + offset, v);
	}
	void write32(size_t offset, uint32_t v) {
		IpPacket::write32(8 + offset, v);
	}
	void write(size_t offset, const void* data, size_t bytes) {
		IpPacket::write(8 + offset, data, bytes);
	}
	void fillChecksum() {
		IpPacket::fillChecksum();
		IpPacket::write16(6, 0); // UDP校验和可以忽略
	}
};

class UdpPacketBuffer: public UdpPacket {
	uint8_t _buf[1500];
public:
	UdpPacketBuffer() :
			UdpPacket(_buf) {
	}
};

}

}
