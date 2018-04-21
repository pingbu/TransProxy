
function $(id) {
	return document.getElementById(id);
}

function httpQuery(method, url, body) {
	var xmlHttp = new XMLHttpRequest();
	xmlHttp.open(method, url, false);
	xmlHttp.send(body);
	if (xmlHttp.status != 200) {
		window.alert(xmlHttp.status + " " + xmlHttp.statusText);
		return null;
	}
	return xmlHttp.responseText;
}

function parseQuery() {
	var r = { };
	var p = location.search.split("&");
	for (var i = 0; i < p.length; ++i) {
		var q = p[i].indexOf("=");
		r[p[i].substring(0, q)] = decodeURI(p[i].substring(q + 1));
	}
	return r;
}

function parseUrl(url) {
	var r = { protocol: null, host: null, port: null, user: null, password: null };
	var a = url.indexOf("://");
	r.protocol = url.substring(0, a);
	url = url.substring(a + 3);
	a = url.indexOf("/");
	if (a >= 0)
		url = url.substring(0, a);
	a = url.indexOf("@");
	if (a >= 0) {
		var t = url.substring(0, a);
		var p = t.indexOf(":");
		if (p < 0) {
			r.user = t;
		} else {
			r.user = t.substring(0, p);
			r.password = t.substring(p + 1);
		}
		url = url.substring(a + 1);
	}
	a = url.indexOf(":");
	if (a < 0) {
		r.host = url;
	} else {
		r.host = url.substring(0, a);
		r.port = parseInt(url.substring(a + 1));
	}
	return r;
}

function isValidIP(value) {
	var p = value.split(".");
	if (p.length != 4)
		return false;
	for (var i = 0; i < 4; ++i) {
		var v = parseInt(p[i]);
		if (isNaN(v) || v < 0 || v > 255)
			return false;
	}
	return true;
}

function isValidPort(value) {
	var v = parseInt(value);
	return !isNaN(v) && v > 0 && v < 65536;
}

function isValidURL(value) {
	var p = value.indexOf("://");
	if (p < 0)
		return false;
	var schema = value.substring(0, p);
	return schema == "http" || schema == "https";
}
