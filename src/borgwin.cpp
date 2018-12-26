// borgwin2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <sstream>

namespace po = boost::program_options;
namespace bp = boost::process;

std::string cygwinDir = R"(c:\cygwin64\bin)";
std::string workingDir = "";
std::string passPhrase = "";
std::string borgEnv = "borg-env";
std::vector<std::string> borgargs;
std::string logFile = "";
bool debug_out = false;
bool dont_run = false;
 
typedef boost::iostreams::tee_device<std::ostream, std::ostream> TeeDevice;
 
std::string convert_pathToUnix(const std::string& winPath)
{
	bp::ipstream p;
	boost::filesystem::path cygpathTool = boost::filesystem::path(cygwinDir) / "cygpath.exe";

	std::vector<std::string> args{ "-u", winPath };
	bp::child c(cygpathTool, args, bp::std_out > p);

	std::string unixPath;
	std::getline(p, unixPath);

	c.wait();
	if (c.exit_code() != 0) throw std::exception("Error running cygpath.");
	return unixPath;
}

std::string quote(const std::string& s)
{
	// test if string has spaces:
	if (s.find(' ') != std::string::npos)
	{
		return "\"" + s + "\"";
	}
	else
		return s;
}

void run_borg()
{
	std::string cmdString;
	cmdString.append("source borg-env/bin/activate && ");
	if (!passPhrase.empty())
		cmdString.append("export BORG_PASSPHRASE='" + passPhrase + "' && ");
	if (!workingDir.empty())
		cmdString.append("cd \"" + workingDir + "\" && ");
	cmdString.append("borg");
	for (auto s : borgargs) {
		cmdString.append(" ");
		cmdString.append(s);
	}

	std::vector<std::string> shellArgs;
	shellArgs.push_back("--login");
	shellArgs.push_back("-c");
	shellArgs.push_back(cmdString);

	if (debug_out) {
		std::cout << "running bash process: " << cmdString << "\n";
	}

	boost::filesystem::path bashPath = boost::filesystem::path(cygwinDir) / "bash.exe";

	if (dont_run == false) {
		//std::ofstream of("test.log");
		//TeeDevice tee(std::cout, of);
		//boost::iostreams::stream<TeeDevice> teeStream(tee);
		
		bp::system(bashPath, shellArgs); //  , bp::std_out > teeStream, bp::std_err > stderr);
	}
}
 
int main(int ac, char* av[])
{
	//{
	//	std::ofstream of("test.log");
	//	TeeDevice tee(std::cout, of);
	//	boost::iostreams::stream<TeeDevice> teeStream(tee);
	//	teeStream << "Hallo Welt";
	//}
	//return 1;

	try {
		// Declare the supported options.
		po::options_description desc(
			"Usage: borgwin [options] -- [borg-arguments...]\n"
			"Available options");
		desc.add_options()
			("help", "produce help message")
			("version", "display borgwin version number")
			("cygwin,c", po::value<std::string>(&cygwinDir)->default_value(cygwinDir), "cygwin installation directory. E.g. c:\\cygwin64\\bin")
			("workdir,d", po::value<std::string>(&workingDir)->default_value(workingDir), "directory from where borg is started.")
			("passphrase,p", po::value<std::string>(&passPhrase)->default_value(passPhrase), "phassphrase for repository (if needed). This option is omitted, the user is prompted to enter the passphrase.")
			("env,e", po::value<std::string>(&borgEnv)->default_value(borgEnv), "Python virtual environment, in which borg is installed.")
			("debug", po::bool_switch(&debug_out), "display debug information.")
			("dont-run,n", po::bool_switch(&dont_run), "doesn't execute borg. Use with --debug to display the resulting bash command line.")
			("log", po::value<std::string>(&logFile)->default_value("")->implicit_value("borg.log"), "writes stdout and stderr to the specified file (in addition to console output)")
			;

		// we want to use -- to separate the borg arguments from borgwin arguments. This is explained in the help 
		// command and should not appear as available option. So we declare -b in a separate description, which
		// is not displayed in the help command, but used for parsing.
		
		po::options_description invisibleDesc("");
		invisibleDesc.add_options()
			("borg-args,b", po::value(&borgargs), "All the options passed on to borg.");


		po::positional_options_description positional;
		positional.add("borg-args", -1);

		// This description is used for parsing and validation
		po::options_description cmdline_options;
		cmdline_options.add(desc).add(invisibleDesc);

		// And this one to display help
		po::options_description visible_options;
		visible_options.add(desc);

		po::parsed_options parsed = po::command_line_parser(ac, av)
			.options(cmdline_options)
			.positional(positional)
			.run();

		po::variables_map vm;
		po::store(parsed, vm);
		po::notify(vm);

		if (vm.count("help")) {
			std::cout << desc << "\n";

			std::cout << "borg-arguments... are arguments to be passed to borg and are separated from\n";
			std::cout << "borgwin's own options by '--'.\n";
			std::cout << "Use $pu(...) to identify paths, which need to be converted to cygwin paths.\n";

			std::cout << "\n";
			std::cout << "Examples:\n\n";
			std::cout << "borgwin -- --version\n";
			std::cout << "   to displays borg version.\n";
			std::cout << R"(borgwin -p "123 4" -- list $pu("h:\borgtest repo"))" << "\n";
			std::cout << R"(borgwin -p "123 4" -- list $pu("h:\borgtest repo")::"Run 1")" << "\n";
			std::cout << "   is translated to:\n";
			std::cout << R"(   borg list "/cygdrive/h/borgtest repo::Run 1")" << "\n";
			std::cout << R"(borgwin -p "123 4" -- create $pu("h:\borg repo")::"Run 2" $pu("h:\borg test"))" << "\n";
			std::cout << "   is translated to:\n";
			std::cout << R"(   borg create "/cygdrive/h/borg repo::Run 2" "/cygdrive/h/borg test")";
			std::cout << "\n";
			return 1;
		}

		if (vm.count("version")) {
			std::cout << "0.1";
			return 1;
		}

		const boost::regex re(R"(\$pu\(([^)]*)\))");

		for (size_t i = 0; i < borgargs.size(); i++) {
			boost::cmatch what;
			boost::match_results<std::string::const_iterator> testMatches;
			std::string oldStr = borgargs[i];
			std::string newStr = boost::regex_replace(
				oldStr, re, [](auto& match)->std::string
			{
				std::string winPath(match[1].first, match[1].second);
				return convert_pathToUnix(winPath);
			});

			if (debug_out) {
				if (oldStr != newStr)
					std::cout << "Replaced '" << oldStr << "' with '" << newStr << "'\n";
			}

			borgargs[i] = quote(newStr);
		}
		if (debug_out) {
			std::cout << "Working dir: '" << workingDir << "'\n";
			std::cout << "Borg env: '" << borgEnv << "'\n";
			std::cout << "cygwin: '" << cygwinDir << "'\n";
			std::cout << "Passphrase: '" << passPhrase << "'\n";
			std::cout << "Log file: '" << logFile << "'\n";
			std::cout << "Borg arguments: \n";
			for (size_t i = 0; i < borgargs.size(); i++) {
				std::cout << i << ": '" << borgargs[i] << "'\n";
			}
		}
		run_borg();
	}
	catch (const std::exception& ex)
	{
		std::cout << "Error: " << ex.what() << "\n";
	}
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
