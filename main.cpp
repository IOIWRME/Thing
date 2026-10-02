#include <string>
#include <print>
#include <memory>
#include <vector>
#include <algorithm>

#include <curl/curl.h>

int
utf8_n_bytes (uint8_t c)
{
	if((c & 0b10000000) == 0) {
		return 1;
	} else if((c & 0b11100000) == 0b11000000) {
		return 2;
	} else if((c & 0b11110000) == 0b11100000) {
		return 3;
	} else if((c & 0b11111000) == 0b11110000) {
		return 4;
	}
	return -1;
}
std::string
to_unicode (auto& it, auto end)
{
	if(it == end) return {};
	int nb = utf8_n_bytes(*it);
	if(nb == -1) return {};
	std::string out;
	for(int i = 0; i < nb; ++i) {
		if(it == end) return {};
		out.push_back(*(it++));
	}
	return out;
}
size_t
ascii_to_digit (auto& it, auto end)
{
	size_t out = 0;
	while(it != end) {
		if(!std::isdigit(*it)) break;
		out *= 10;
		out += (*it - '0');
		++it;
	}
	return out;
}
static size_t
fill_buf(char* data, size_t size, size_t nmemb, void* str_p)
{
	std::string& str = *reinterpret_cast<std::string*>(str_p);
	for(size_t i = 0; i < nmemb; ++i) {
		if(std::isalpha(data[i]) || data[i] == '-') continue;
		str += data[i];
	}
	return nmemb;
}
void
eat_ws (auto& it, auto end)
{
	while(it != end) {
		if(!std::isspace(*it)) break;
		++it;
	}
}
struct coord {
	size_t x;
	size_t y;
	std::string c;
};
std::vector<coord>
parse_file (auto it, auto end)
{
	std::vector<coord> grid;
	while(it != end) {
		auto save = it;
		if(auto tmp = to_unicode(it, end); tmp.size() != 3 || !(tmp[0] == 0xbf && tmp[1] == 0xbb && tmp[2] == 0xef)) {
			save = it;
		}
		eat_ws(it, end);
		if(!std::isdigit(*it)) { std::println("XBREAK"); break; }
		size_t x = ascii_to_digit(it, end);
		eat_ws(it, end);
		std::string c = to_unicode(it, end);
		if(c.empty()) { std::println("CBREAK"); break; }
		eat_ws(it, end);
		if(!std::isdigit(*it)) { std::println("YBREAK"); break; }
		size_t y = ascii_to_digit(it, end);
		grid.emplace_back(x, y, std::move(c));
	}
	return grid;
}
bool
google_doc_to_grid (std::string const& dirty_url)
{
	std::string url;
	{
		std::unique_ptr<CURLU, decltype(&curl_url_cleanup)> url_handle(curl_url(), &curl_url_cleanup);
		if(url_handle == nullptr) return false;
		curl_url_set(url_handle.get(), CURLUPART_URL, dirty_url.data(), 0);
		curl_url_set(url_handle.get(), CURLUPART_QUERY, "format=txt", 0);
		char* new_url = NULL;
		curl_url_get(url_handle.get(), CURLUPART_URL, &new_url, 0);
		url = new_url;
		curl_free(new_url);
	}
	std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), &curl_easy_cleanup);
	if(curl == nullptr) return false;
	curl_easy_setopt(curl.get(), CURLOPT_URL, url.data());
	curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, fill_buf);
	curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1);
	std::string file;
	curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, (void*)&file);
	if(curl_easy_perform(curl.get()) != CURLE_OK) return false;

	std::vector<coord> grid = parse_file(file.begin(), file.end());
	if(grid.empty()) return false;
	std::sort(grid.begin(), grid.end(), [](coord const& a, coord const& b) { return a.x < b.x; });
	std::stable_sort(grid.begin(), grid.end(), [](coord const& a, coord const& b) { return a.y < b.y; });
	size_t x = 0;
	size_t y = 0;
	for(auto const& g : grid) {
		while(g.y != y) {std::println(""); x = 0; ++y;}
		while(g.x > x) {std::print(" "); ++x;}
		std::print("{}", g.c);
		++x;
	}
	std::println("");
	return true;
}

int
main (int argc, char** argv)
{
	google_doc_to_grid("https://docs.google.com/document/d/1ulJFfuGjgwG3yMOO5CzzmxKZTiwHIZkbN-k2eosSirc/export?format=pdf&format=txt");
}
