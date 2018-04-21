#include "Base/Debug.h"
#include "HTTP.h"

namespace TransProxy {

class MallocHTTP: public HttpService {
public:
	static void startLog();
	bool onHttpRequest(Net::HttpRequest& request, Net::HttpResponse& response)
			THROWS;
};

}
