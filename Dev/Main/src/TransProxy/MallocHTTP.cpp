#define LOG_TAG  "MallocHTTP"

#include "Base/Debug.h"
#include "Base/Utils.h"
#include "DomainResolver.h"
#include "MallocHTTP.h"

namespace TransProxy {

static const char* MALLOC_TAG = "MallocHTTP";

void MallocHTTP::startLog() {
	::setDefaultMallocTag(MALLOC_TAG);
}

bool MallocHTTP::onHttpRequest(Net::HttpRequest& request,
		Net::HttpResponse& response) THROWS {
	Utils::String path = request.getPath();
	if (path == "/debug/malloc.html") {
		response.setStatus(200, "OK");
		response.setContentType("text/html");
		response.printf(
				"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />");
		response.printf("<title>Malloc</title>");
		response.printf(
				"<table border=\"1\" bordercolor=\"lightgrey\" style=\"border-collapse: collapse\">");
		size_t totalBytes = 0;
		char t[32], s[32];
		for (MallocBlock* block = __firstMallocBlock; block; block =
				block->next)
			if (block->tag == MALLOC_TAG) {
				totalBytes += block->bytes;
				Utils::formatTime(t, block->time);
				Utils::formatSize(s, block->bytes);
				response.printf("<tr>");
				response.printf("<td align=\"right\">%s</td>", t);
				response.printf("<td align=\"right\">%s</td>", s);
				response.printf("<td>%s</td>", block->file);
				response.printf("<td>#%u</td>", block->line);
				response.printf("</tr>");
			}
		Utils::formatSize(s, totalBytes);
		response.printf("<tr>");
		response.printf("<th>Total</th>");
		response.printf("<th align=\"right\">%s</th>", s);
		response.printf("<td></td>");
		response.printf("<td></td>");
		response.printf("</tr>");
		response.printf("</table>");
		return true;
	}
	return HttpService::onHttpRequest(request, response);
}

}
