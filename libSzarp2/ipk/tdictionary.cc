/* 
  SZARP: SCADA software 

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "szarp_config.h"
#include "conversion.h"

bool match_text(const std::wstring& expr, const std::wstring& text, std::vector<std::wstring>& matched);

TDictionary::TDictionary(const std::wstring& szarp_dir) {

#if BOOST_FILESYSTEM_VERSION == 3
	m_dictionary_path = (boost::filesystem::wpath(szarp_dir) / L"resources" / L"dictionary.xml").wstring();
#else
	m_dictionary_path = (boost::filesystem::wpath(szarp_dir) / L"resources" / L"dictionary.xml").string();
#endif

}

void TDictionary::LoadDictionary(const std::wstring &from_lang, const std::wstring& to_lang) {
	xmlDocPtr doc = xmlParseFile(SC::S2A(m_dictionary_path).c_str());
	if (doc == NULL)
		return;

	if (doc->children == NULL) {
		xmlFreeDoc(doc);
		return;
	}

	for (xmlNodePtr n = doc->children->children; n; n = n->next) {
		if (!n->name)
			continue;

		std::wstring section_name = SC::U2S(n->name);

		for (xmlNodePtr n2 = n->children; n2; n2 = n2->next) {
			if (xmlStrcmp(n2->name, BAD_CAST "entry"))
				continue;

			bool f = false;
			bool t = false;
			std::pair<std::wstring, std::wstring> e;

			for (xmlNodePtr n3 = n2->children; n3; n3 = n3->next) {
				if (xmlStrcmp(n3->name, BAD_CAST "translation"))
					continue;

				xmlChar* _lang = xmlGetNoNsProp(n3, BAD_CAST "lang");
				assert(_lang);

				std::wstring lang = SC::U2S(_lang);

				if (from_lang == lang
						|| to_lang == lang) {

					xmlChar *v = xmlGetNoNsProp(n3, BAD_CAST "value");

					if (from_lang == lang) {
						f = true;
						e.first = SC::U2S(v);
					} else {	
						t = true;
						e.second = SC::U2S(v);
					}

					xmlFree(v);
				}
				xmlFree(_lang);
			}

			if (f && t)
				m_dictionary[section_name].push_back(e);
		}
	}
}

std::wstring TDictionary::TranslateEntry(const std::wstring &section, const std::wstring &sstring) {
	std::vector<ENTRY>& es = m_dictionary[section];

	std::vector<std::wstring> matched;
	for (size_t i = 0; i < es.size(); i++) {
		if (!match_text(es[i].first, sstring, matched))
			continue;

		std::wstring res = es[i].second;

		if (res.empty())
			continue;

		size_t j = 0;
		size_t k = 0;
		while ((j = res.find(L'*', j)) != std::wstring::npos) {
			if ((j + 1) < res.size()
					&& res[j + 1] == L'*') {
				j += 2;
				continue;
			}

			if (k == matched.size()) {
				matched.clear();
				goto failed;
			}

			res.replace(j, 1, matched[k++]);
		}

		if (k == matched.size())
			return res;
failed:;
	}

	if (section == L"common")
		return sstring;

	return TranslateEntry(L"common", sstring);
}

void TDictionary::TranslateParamName(TParam *p) {
	const std::wstring& pn = p->GetName();
	std::wstring r;

	size_t p1 = 0, p2;

	while ((p2 = pn.find(L':', p1)) != std::wstring::npos) {
		r += TranslateEntry(L"param_name", pn.substr(p1, p2 - p1)) + L":";
		p1 = p2 + 1;
	}

	r += TranslateEntry(L"param_name", pn.substr(p1));

	p->SetTranslatedName(r);
}

void TDictionary::TranslateParam(TParam *p) {
	TranslateParamName(p);
	p->SetTranslatedShortName(TranslateEntry(L"short_name", p->GetShortName()));
	p->SetTranslatedDrawName(TranslateEntry(L"draw_name", p->GetDrawName()));

	for (TDraw *d = p->GetDraws(); d; d = d->GetNext())
		d->SetTranslatedWindow(TranslateEntry(L"draw_title", d->GetWindow()));

	for (TRaport *r = p->GetRaports(); r; r = r->GetNext())
		r->SetTranslatedTitle(TranslateEntry(L"raport_title", r->GetTitle()));

}

void TDictionary::TranslateIPK(TSzarpConfig *ipk, const std::wstring& tolang) {
	LoadDictionary(SC::A2S(ipk->getAttribute<std::string>("language", "pl")), tolang);

	ipk->SetTranslatedTitle(TranslateEntry(L"title", ipk->GetTitle()));

	for (TParam* p = ipk->GetFirstParam(); p; p = p->GetNextGlobal())
		TranslateParam(p);

	m_dictionary.clear();
}

bool match_text(const std::wstring& expr, size_t i, 
		const std::wstring& text, size_t j, 
		std::vector<std::wstring>& matched);

bool match_asterisk(const std::wstring& expr, size_t ei, 
			const std::wstring& text, size_t ej, 
			std::vector<std::wstring>& matched) {
	size_t i = ei + 1;

	if (i == expr.size() || ej == text.size()) {
		matched.push_back(text.substr(ej));
		return true;
	}

	while (true) {
		wchar_t c = expr[i];
		for (size_t j = ej; j < text.size(); ++j) {
			if (c != text[j]) 
				continue;

			std::vector<std::wstring> _matched;
			if (!match_text(expr, i, text, j, _matched))
				continue;

			matched.push_back(text.substr(ej, j - ej));
			for (size_t k = 0; k < _matched.size(); ++k)
				matched.push_back(_matched[k]);

			return true;
		
		}

		return false;
	}

}

bool match_text(const std::wstring& expr, size_t i, 
		const std::wstring& text, size_t j, 
		std::vector<std::wstring>& matched) {

	while (true) {
		if (i == expr.size()) {
			if (j == text.size()) {
				return true;
			} else {
				return false;
			}
		}

		wchar_t c = expr[i];
		if (c == L'*') {
			if ((i + 1) == expr.size() || (expr[i + 1] != L'*'))
				return match_asterisk(expr, i, text, j, matched);
			i++;
		}

		if (c != text[j])
			return  false;
		j++;
		i++;

	}

}

bool match_text(const std::wstring& expr, const std::wstring& text, std::vector<std::wstring>& matched) {
	return match_text(expr, 0, text, 0, matched);
}

