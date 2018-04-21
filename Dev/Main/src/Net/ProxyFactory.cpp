#include "Base/Utils.h"
#include "Net/IPv4.h"
#include "ProxyDirect.h"
#include "ProxyHttp.h"
#include "ProxySocks.h"
#include "ProxyFactory.h"

namespace Net {

TcpClientFactory* newProxy(TcpClientFactory* factory, const char* url) THROWS {
	if (url == NULL)
		return new ProxyDirect(factory);

	Utils::String s = url;
	if (s.startsWith("http://"))
		return new ProxyHttp(factory, IPv4::SockAddr(url + 7));
	else if (s.startsWith("sock://"))
		return new ProxySocks(factory, IPv4::SockAddr(url + 7));
	else if (s.startsWith("socks://") || s.startsWith("sock5://"))
		return new ProxySocks(factory, IPv4::SockAddr(url + 8));

	THROW(new Utils::Exception("Invalid proxy URL '%s'", url));
	return NULL;
}

}
