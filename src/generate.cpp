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
	"419-scam",
	"420",
	"500-dollar-cash-prize",
	"abuse",
	"adblock-bypass",
	"ads",
	"advert",
	"ambien",
	"anarchist-police-takeover",
	"anarchy",
	"antifa",
	"audio",
	"bad-mixtape",
	"badonkadonk",
	"bank-transfer",
	"bbw",
	"beatdown",
	"beating-women",
	"big-bang",
	"big-cocks",
	"big-milf",
	"big-willie",
	"bitcoin-2x",
	"bitcoin-cash-paydirt",
	"bitcoin-miner",
	"buproprion",
	"chop",
	"cialis",
	"cocaine",
	"crack",
	"crime-tips",
	"crypto",
	"daesh",
	"darkweb",
	"detector",
	"diazepam",
	"doctor",
	"dog-sex",
	"donkey-cock",
	"drive-by-shooting",
	"drugs",
	"effexor",
	"etherium-multiplier",
	"execute",
	"fake",
	"fap",
	"fappening",
	"final",
	"flame",
	"flash",
	"gangbang",
	"gmail",
	"goatse",
	"gone-wild",
	"gps",
	"gps-location",
	"gun-fight",
	"hacker",
	"he-dies",
	"he-fux-her",
	"heroin",
	"homeless-death",
	"homeless-man",
	"homeless-woman",
	"hood-shooting",
	"horse-sex",
	"hotmail",
	"i-make-2000-week-sitting-at-home",
	"install",
	"internet",
	"interview",
	"ip-finder",
	"ip-stealer",
	"isis",
	"jailbait",
	"java",
	"journalist",
	"keygen",
	"keylog",
	"keylogger",
	"knife-fight",
	"levitra",
	"lexapro",
	"locator",
	"loli",
	"lolicon",
	"lorazepam",
	"lsd",
	"male-enhancement",
	"malware",
	"marijuana",
	"milf-69",
	"mine-coins-for-free",
	"mommy-milkers",
	"monster-erection",
	"my-tits-are-legend",
	"no-holds-barred-beatdown",
	"password",
	"penetration",
	"penis-enlargement",
	"petit-milf",
	"phishing",
	"physical-removal",
	"pig-sex",
	"pirated-movies",
	"pirated-music",
	"pizzagate",
	"popup",
	"prozac",
	"pussy",
	"pwn",
	"qanon",
	"rat-sex",
	"risperidol",
	"root",
	"scam",
	"seroquel",
	"sex",
	"sexwithcats",
	"sexwithdogs",
	"she-dies",
	"she-fux-him",
	"shit",
	"shooting",
	"shoplifting",
	"snuff",
	"sound",
	"spam",
	"spyware",
	"steal",
	"stoned",
	"stoned-for-adultery",
	"subway-death",
	"taliban-interview",
	"taliban-meetup",
	"taliban-recruiter",
	"terrorist",
	"the-donald",
	"they-hurt-her",
	"torrent",
	"trojan",
	"underground-death",
	"unlocker",
	"viagra",
	"video",
	"virus",
	"weed",
	"weight-loss",
	"wet-pussy",
	"wife-pussy",
	"woman-beaten",
	"woman-gets-stoned",
	"worldstar-fight",
};

static const std::array ext =
{
	".avi",
	".bat",
	".csv",
	".divx",
	".doc",
	".docx",
	".exe",
	".gif",
	".ini",
	".jpg",
	".m4a",
	".mkv",
	".mov",
	".mp3",
	".mp4",
	".pdf",
	".png",
	".ppt",
	".txt",
	".xls",
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
