#include <boost/algorithm/string/join.hpp>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <string>
#include <array>
#include <random>

namespace generate
{

static const std::array nsfw =
{
	"antifa",
	"ambien",
	"milf-69",
	"they-hurt-her",
	"subway-death",
	"underground-death",
	"homeless-death",
	"shit",
	"homeless-man",
	"homeless-woman",
	"big-cocks",
	"weed",
	"crack",
	"sex",
	"drugs",
	"viagra",
	"cialis",
	"bitcoin-2x",
	"bitcoin-miner",
	"mine-coins-for-free",
	"etherium-multiplier",
	"terrorist",
	"journalist",
	"doctor",
	"abuse",
	"levitra",
	"taliban-meetup",
	"taliban-recruiter",
	"taliban-interview",
	"interview",
	"isis",
	"daesh",
	"knife-fight",
	"gun-fight",
	"worldstar-fight",
	"drive-by-shooting",
	"no-holds-barred-beatdown",
	"beatdown",
	"she-fux-him",
	"he-fux-her",
	"big-willie",
	"sexwithdogs",
	"sexwithcats",
	"beating-women",
	"the-donald",
	"pizzagate",
	"physical-removal",
	"shoplifting",
	"crime-tips",
	"anarchy",
	"anarchist-police-takeover",
	"chop",
	"fappening",
	"he-dies",
	"she-dies",
	"wife-pussy",
	"jailbait",
	"trojan",
	"fap",
	"donkey-cock",
	"pirated-movies",
	"pirated-music",
	"torrent",
	"darkweb",
	"horse-sex",
	"pig-sex",
	"dog-sex",
	"rat-sex",
	"bank-transfer",
	"419-scam",
	"scam",
	"my-tits-are-legend",
	"big-bang",
	"gangbang",
	"big-milf",
	"petit-milf",
	"goatse",
	"loli",
	"lolicon",
	"bbw",
	"bad-mixtape",
	"mommy-milkers",
	"badonkadonk",
	"hood-shooting",
	"shooting",
	"video",
	"audio",
	"sound",
	"keylogger",
	"spyware",
	"steal",
	"phishing",
	"internet",
	"password",
	"java",
	"flame",
	"penetration",
	"gmail",
	"hotmail",
	"qanon",
	"fake",
	"detector",
	"keylog",
	"popup",
	"advert",
	"ads",
	"adblock-bypass",
	"flash",
	"malware",
	"spam",
	"install",
	"virus",
	"keygen",
	"unlocker",
	"ip-stealer",
	"ip-finder",
	"locator",
	"gps-location",
	"gps",
	"crypto",
	"root",
	"pwn",
	"execute",
	"gone-wild",
	"male-enhancement",
	"penis-enlargement",
	"monster-erection",
	"weight-loss",
	"hacker",
	"bitcoin-cash-paydirt",
	"i-make-2000-week-sitting-at-home",
	"final",
	"lorazepam",
	"diazepam",
	"prozac",
	"effexor",
	"lexapro",
	"seroquel",
	"risperidol",
	"buproprion",
	"heroin",
	"lsd",
	"cocaine",
	"marijuana",
	"420",
	"stoned",
	"woman-gets-stoned",
	"stoned-for-adultery",
	"snuff",
};

static const std::array ext =
{
	".ini",
	".exe",
	".bat",
	".pdf",
	".jpg",
	".mov",
	".mp3",
	".mp4",
	".csv",
	".xls",
	".doc",
	".docx",
	".ppt",
	".txt",
	".png",
	".gif",
	".mkv",
	".avi",
	".m4a",
	".divx",
};

static inline std::string
generate_randstr(std::mt19937& mt)
{
	const std::string charlist{"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_"};
	static std::uniform_int_distribution<std::size_t> dist_randstr(0, charlist.size() - 1);
	static std::uniform_int_distribution<std::size_t> dist_len(6, 15);
	std::ostringstream os;

	for(std::size_t i = 0; i < dist_len(mt); i++)
	{
		os << charlist[dist_randstr(mt)];
	}

	return os.str();
};

std::string
generate_random_filename()
{
	std::vector<std::string> out;
	std::random_device rd;
	std::mt19937 mt{rd()};
	std::uniform_int_distribution<std::size_t> dist_nsfw(0, nsfw.size() - 1);
	std::uniform_int_distribution<std::size_t> dist_ext(0, ext.size() - 1);
	std::uniform_int_distribution<std::size_t> dist_len(5, 8);
	std::uniform_int_distribution<std::size_t> dist_insert_random(0, 4);

	std::size_t len = dist_len(mt);
	std::size_t random_insert_count = 0;
	for(std::size_t i = 0; i < len; i++)
	{
		if(random_insert_count < 3 && dist_insert_random(mt) == 0)
		{
			random_insert_count++;
			out.push_back(generate_randstr(mt));
		}
		else
		{
			out.push_back(nsfw[dist_nsfw(mt)]);
		}
	}

	if(!random_insert_count)
		out.push_back(generate_randstr(mt));

	std::ostringstream os;
	os << boost::algorithm::join(out, "-");
	os << ext[dist_ext(mt)];
	return os.str();
}

} // namespace generate
