#include "hlsparser.h"
#include <vector>
#include <string.h>
 
using namespace std;
 
 
namespace Parser 
{
  //将字符串分割成多个字符串的vector
  std::vector<std::string> split(const std::string& s, const char *delim)
  {
    std::vector<std::string> ret;
    int last = 0;
    int index = s.find(delim, last);
    while (index != string::npos) 
      {
        if (index - last > 0) 
          {
            ret.push_back(s.substr(last, index - last));
          }
        last = index + strlen(delim);
        index = s.find(delim, last);
      }
    if (!s.size() || s.size() - last > 0) 
      {
        ret.push_back(s.substr(last));
      }
    return ret;
  }
  //去除前后的空格、回车符、制表符...
  std::string& trim(std::string &s, const std::string &chars = " \r\n\t\"")
  {
    string map(0xFF, '\0');
    for (auto &ch : chars) 
      {
        map[(unsigned char &)ch] = '\1';
      }
    while (s.size() && map.at((unsigned char &)s.back())) s.pop_back();
    while (s.size() && map.at((unsigned char &)s.front())) s.erase(0, 1);
    return s;
  }
 
 
  //不分大小写的字符串比较函数
  struct StrCaseCompare 
  {
    bool operator()(const string &__x, const string &__y) const 
    {
      #ifdef WIN32
      return _stricmp(__x.data(), __y.data()) < 0;
      #else
      return strcasecmp(__x.data(), __y.data()) < 0;//<string.h>
      #endif
    }
  };
  class StrCaseMap : public multimap<std::string, std::string, StrCaseCompare>
  {
  public:
    typedef multimap<string, string, StrCaseCompare> Super;
    StrCaseMap() = default;
    ~StrCaseMap() = default;
 
    string &operator[](const string &k) 
    {
      auto it = find(k);
      if (it == end())
        {
          it = Super::emplace(k, "");
        }
      return it->second;
    }
 
    template <typename V>
    void emplace(const string &k, V &&v) 
    {
      auto it = find(k);
      if (it != end()) 
        {
          return;
        }
      Super::emplace(k, std::forward<V>(v));
    }
 
    template <typename V>
    void emplace_force(const string k, V &&v) 
    {
      Super::emplace(k, std::forward<V>(v));
    }
  };
  string FindField(const char* buf, const char* start, const char *end, int bufSize = 0) 
  {
    if (bufSize <= 0) 
      {
        bufSize = strlen(buf);
      }
    const char *msg_start = buf, *msg_end = buf + bufSize;
    int len = 0;
    if (start != NULL) 
      {
        len = strlen(start);
        msg_start = strstr(buf, start);
      }
    if (msg_start == NULL) 
      {
        return "";
      }
    msg_start += len;
    if (end != NULL) 
      {
        msg_end = strstr(msg_start, end);
        if (msg_end == NULL) 
          {
            return "";
          }
      }
    return string(msg_start, msg_end);
  }
  StrCaseMap parseArgs(const std::string &str, const char *pair_delim="&", const char *key_delim="=")
  {
    StrCaseMap ret;
    auto arg_vec = Parser::split(str, pair_delim);
    for (string &key_val : arg_vec) 
      {
        auto key = FindField(key_val.data(), NULL, key_delim);
        auto val = FindField(key_val.data(), key_delim, NULL);
        ret.emplace_force(trim(key), trim(val));
      }
    return ret;
  }
 
};
 
 
 
 
 
 
 
 
 
 
 
 
bool HlsParser::parse(const string &http_url, const string &m3u8, vector<ts_segment> &p_v) 
{
  float extinf_dur = 0;
  ts_segment segment;
  //  map<int, ts_segment> ts_map;
  _total_dur = 0;
  _is_live = true;
  _is_m3u8_inner = false;
  //  int index = 0;
 
  auto lines = Parser::split(m3u8, "\n");
  for (auto &line : lines) 
    {
      Parser::trim(line);
      if (line.size() < 2) {
        continue;
      }
      /*
      if ((_is_m3u8_inner || extinf_dur != 0) && line[0] != '#') {
        segment.duration = extinf_dur;
        segment.sn = -1;
        segment.pn = -1;
        if (line.find("http://") == 0 || line.find("https://") == 0) {
          segment.url = line;
        } else {
          if (line.find("/") == 0) {
            segment.url = http_url.substr(0, http_url.find("/", 8)) + line;
          } else {
            segment.url = http_url.substr(0, http_url.rfind("/") + 1) + line;
          }
        }
        if (!_is_m3u8_inner) {
          size_t sn_end = line.find(".");
          size_t pn_end = line.rfind(".");
          size_t sn_start = line.rfind("_");
          string sn = line.substr(sn_start+1, sn_end - sn_start - 1);
          string pn = line.substr(sn_end + 1, pn_end-sn_end - 1);
          segment.sn = atoi(sn.c_str());
          segment.pn = atoi(pn.c_str());
          //ts按照先后顺序排序
          ts_map.emplace(index++, segment);
        } else {
          //子m3u8按照带宽排序
          ts_map.emplace(segment.bandwidth, segment);
        }
        extinf_dur = 0;
        continue;
      }
      */
      _is_m3u8_inner = false;
      if (line.find("#EXTINF:") == 0) {
        sscanf(line.data(), "#EXTINF:%f,", &extinf_dur);
        _total_dur += extinf_dur;
        continue;
      }
      static const string s_stream_inf = "#EXT-X-STREAM-INF:";
      if (line.find(s_stream_inf) == 0) {
        _is_m3u8_inner = true;
        auto key_val = Parser::parseArgs(line.substr(s_stream_inf.size()), ",", "=");
        segment.program_id = atoi(key_val["PROGRAM-ID"].data());
        segment.bandwidth = atoi(key_val["BANDWIDTH"].data());
        sscanf(key_val["RESOLUTION"].data(), "%dx%d", &segment.width, &segment.height);
        continue;
      }
      static const string server_control = "#EXT-X-SERVER-CONTROL:";
      if (line.find(server_control) == 0 ) {
        auto key_val =  Parser::parseArgs(line.substr(server_control.size()), ",", "=");
        _can_block_reload = key_val["CAN-BLOCK-RELOAD"].data() == "YES";
        sscanf(key_val["PART-HOLD-BACK"].data(), "%f", &_part_hold_back);
        continue;
      }
      static const string part = "#EXT-X-PART:";
      if (line.find(part) == 0) {
        auto key_val = Parser::parseArgs(line.substr(part.size()), ",", "=");
        segment.duration = atoi(key_val["DURATION"].data());
        string uri = key_val["URI"].data();
        segment.url =  http_url.substr(0, http_url.rfind("/") + 1) + uri;
        //parse LLHLS_Video3_90204416.2.mp4 into sn and pn
        size_t sn_end = uri.find(".");
        size_t pn_end = uri.rfind(".");
        size_t sn_start = uri.rfind("_");
        string sn = uri.substr(sn_start+1, sn_end - sn_start - 1);
        string pn = uri.substr(sn_end + 1, pn_end-sn_end - 1);
        segment.sn = atoi(sn.c_str());
        segment.pn = atoi(pn.c_str());
        if (p_v.empty()) {
            p_v.push_back(segment);
        }
        else {
          // only append partial segments if not yet seen
          ts_segment l = p_v[p_v.size()-1];
          if (segment.sn == l.sn && segment.pn > l.pn || segment.sn > l.sn) {
            p_v.push_back(segment);
          }
        }
        continue;
      }
      if (line == "#EXTM3U") {
        _is_m3u8 = true;
        continue;
      }
 
      if (line.find("#EXT-X-ALLOW-CACHE:") == 0) {
        _allow_cache = (line.find(":YES") != string::npos);
        continue;
      }
 
      if (line.find("#EXT-X-VERSION:") == 0) {
        sscanf(line.data(), "#EXT-X-VERSION:%d", &_version);
        continue;
      }
 
      if (line.find("#EXT-X-TARGETDURATION:") == 0) {
        sscanf(line.data(), "#EXT-X-TARGETDURATION:%d", &_target_dur);
        continue;
      }
 
      if (line.find("#EXT-X-MEDIA-SEQUENCE:") == 0) {
        sscanf(line.data(), "#EXT-X-MEDIA-SEQUENCE:%lld", &_sequence);
        continue;
      }
 
      if (line.find("#EXT-X-ENDLIST") == 0) {
        //点播
        _is_live = false;
        continue;
      }
      continue;
    }
 
  if (_is_m3u8) 
    {
      //onParsed(_is_m3u8_inner, _sequence, ts_map);
    }
  /*
  if (!_is_m3u8_inner) 
    {
      std::vector<std::string> tsVec;

      for (auto &item : ts_map)
        {
          auto &segment = item.second;
          tsVec.push_back(item.second.url);
          printf("media segement:%s, %d, %d\n", item.second.url.c_str(), item.second.sn, item.second.pn);
        }
    }
  else 
    {
        auto new_url = ts_map.rbegin()->second.url;
        printf("hls redirect:%s\n", new_url.c_str());
        //        rediret(new_url);
    }
  */
  return _is_m3u8;
}
 
bool HlsParser::isM3u8() const {
  return _is_m3u8;
}
 
bool HlsParser::isLive() const{
  return _is_live;
}
 
bool HlsParser::allowCache() const {
  return _allow_cache;
}
 
int HlsParser::getVersion() const {
  return _version;
}
 
int HlsParser::getTargetDur() const {
  return _target_dur;
}
 
int HlsParser::getSequence() const {
  return _sequence;
}
 
bool HlsParser::isM3u8Inner() const {
  return _is_m3u8_inner;
}
 
/* 
class HlsGetter: public HttpClientImp, HlsParser
{
private:
  string m_m3u8Str;
  int64_t m_recvedSize;
 
public:
  //获取http body
  virtual void onResponseBody(const char *buf, int64_t datalen, int64_t totalSize) override
  {
    m_m3u8Str.append(buf, datalen);
    m_recvedSize += datalen;
    
 
    //解析m3u8，回调函数在onParsed()
    int ret = HlsParser::parse(m_url, m_m3u8Str);
    if (!ret)
      {
        printf("m3u8 parse failed: %s\n", m_m3u8Str.c_str());
      }
 
 
  }
  //处理m3u8的结果
  virtual void onParsed(bool is_m3u8_inner, int64_t sequence, const std::map<int, ts_segment> &ts_map) override
  {
    if (!is_m3u8_inner)
      {
        std::vector<std::string> tsVec;
 
        for (auto &item : ts_map)
          {
            auto &segment = item.second;
            tsVec.push_back(item.second.url);
          }
        
        sucess(tsVec);
      }
    else//m3u8列表
      {
        //选择第一个节目(最高清的?)
        if (ts_map.size() == 0)
          {
            failed(-1);
            return;
          }
        auto new_url = ts_map.rbegin()->second.url;
        printf("hls redirect:%s\n", new_url.c_str());
        rediret(new_url);
      }
  }
}
 
*/
