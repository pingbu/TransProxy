#include <stdio.h>
#include <unistd.h>
#include "Debug.h"
#include "String.h"
#include "Map.h"

#pragma once

namespace Utils {

class IniFile {
	String _file, _tmpFile, _bakFile, _currentGroup;
	Map<String, StringMapItem<String> > _contents;

	void _addGroup(const char* name) {
		_currentGroup = name;
	}
	void _addValueInCurrentGroup(const char* key, const char* value) {
		if (_currentGroup)
			_addValue(String::format("%s/%s", _currentGroup.sz(), key).sz(),
					value);
	}
	void _addValue(const char* key, const char* value) {
		_contents.add(new StringMapItem<String>(key, value));
	}

public:
	IniFile(const char* file) :
			_file(file), _tmpFile(_file + ".tmp"), _bakFile(_file + ".bak"), _currentGroup(
			NULL) THROWS {
		if (!::access(_file, F_OK) && ::access(_bakFile, F_OK))
			::rename(_bakFile, _file);
		::remove(_tmpFile);
		::remove(_bakFile);
		FILE* fp = ::fopen(_file, "rt");
		if (fp) {
			char line[256];
			while (::fgets(line, sizeof(line) - 1, fp)) {
				char* p = ::strchr(line, '#');
				if (!p)
					p = line + ::strlen(line);
				while (p > line && p[-1] > '\0' && p[-1] < ' ')
					--p;
				*p = '\0';
				p = line;
				while (*p > '\0' && *p < ' ')
					++p;
				if (*p) {
					if (*p == '[') {
						char* q = p + ::strlen(p) - 1;
						if (*q == ']') {
							*q = '\0';
							_addGroup(p + 1);
							continue;
						}
					}
					char* q = ::strchr(p, '=');
					if (q) {
						*q++ = '\0';
						_addValueInCurrentGroup(p, q);
					}
				}
			}
			::fclose(fp);
		}
		_currentGroup = NULL;
	}

	const char* getValue(const char* group, const char* key,
			const char* defaultValue = NULL) {
		String k = String::format("%s/%s", group, key);
		StringMapItem<String>* item = _contents.get(k);
		if (item != NULL)
			return item->getValue().sz();
		if (defaultValue)
			_addValue(k, defaultValue);
		return defaultValue;
	}

	void setValue(const char* group, const char* key, const char* value) {
		String k = String::format("%s/%s", group, key);
		_contents.remove(k);
		if (value)
			_addValue(k, value);
	}

	void save() THROWS {
		FILE* fp = ::fopen(_tmpFile, "wt");
		THROW_IF(fp == NULL, new Exception("FAILED to save ini file"));
		String group = "/";
		const char* group_sz = group.sz();
		size_t groupLen = group.length();
		for (StringMapItem<String>* item = _contents.min(); item; item =
				_contents.bigger(item)) {
			String key = item->getKey();
			const char* key_sz = key.sz();
			if (::strncmp(key_sz, group_sz, groupLen) != 0) {
				groupLen = ::strchr(key_sz, '/') - key_sz;
				group = key.substring(0, groupLen);
				::fprintf(fp, "[%s]\n", key.substring(0, groupLen).sz());
				group += '/';
				group_sz = group.sz();
				++groupLen;
			}
			::fprintf(fp, "%s=%s\n", key_sz + groupLen, item->getValue().sz());
		}
		::fclose(fp);
		::rename(_file, _bakFile);
		::rename(_tmpFile, _file);
		::remove(_bakFile);
	}
};

}
