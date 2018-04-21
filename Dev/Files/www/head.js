
var HttHead, HttTail;

function loadHtt(htt) {
	var htt = httpQuery("GET", htt);
	var p = htt.indexOf("${CONTENT}");
	HttHead = htt.substring(0, p);
	HttTail = htt.substring(p + 10);
}

if (screen.width < 640)
	loadHtt("mobile.htt");
else
	loadHtt("default.htt");

document.write(HttHead);
