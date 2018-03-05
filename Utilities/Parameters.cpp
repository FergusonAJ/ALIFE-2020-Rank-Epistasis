//  MABE is a product of The Hintze Lab @ MSU
//     for general research information:
//         hintzelab.msu.edu
//     for MABE documentation:
//         github.com/Hintzelab/MABE/wiki
//
//  Copyright (c) 2015 Michigan State University. All rights reserved.
//     to view the full license, visit:
//         github.com/Hintzelab/MABE/wiki/License

#include<regex>
#include "Parameters.h"

std::shared_ptr<ParametersTable> Parameters::root;

long long ParametersTable::nextTableID = 0;

template <> inline const bool ParametersEntry<bool>::getBool() { return get(); }

template <> inline const std::string ParametersEntry<std::string>::getString() {
  return get();
}

template <> inline const int ParametersEntry<int>::getInt() { return get(); }

template <> inline const double ParametersEntry<double>::getDouble() {
  return get();
}

std::shared_ptr<ParameterLink<bool>>
Parameters::getBoolLink(const std::string &name, std::shared_ptr<ParametersTable> table) {
  auto entry = table->lookupBoolEntry(name);
  auto newLink = std::make_shared<ParameterLink<bool>>(name, entry, table);
  return newLink;
}

std::shared_ptr<ParameterLink<std::string>>
Parameters::getStringLink(const std::string &name,
                          std::shared_ptr<ParametersTable> table) {
  auto entry = table->lookupStringEntry(name);
  auto newLink = std::make_shared<ParameterLink<std::string>>(name, entry, table);
  return newLink;
}

std::shared_ptr<ParameterLink<int>> Parameters::getIntLink(const std::string& name, std::shared_ptr<ParametersTable> table) {
	auto entry = table->lookupIntEntry(name);
	auto newLink = std::make_shared<ParameterLink<int>>(name, entry, table);
	return newLink;
}

std::shared_ptr<ParameterLink<double>> Parameters::getDoubleLink(const std::string& name, std::shared_ptr<ParametersTable> table) {
	auto entry = table->lookupDoubleEntry(name);
	auto newLink = std::make_shared<ParameterLink<double>>(name, entry, table);
	return newLink;
}
/*
void Parameters::parseFullParameterName(const string& fullName, string& nameSpace, string& category, string& parameterName) {
	int i = fullName.size() - 1;
	nameSpace = "";
	category = "";
	parameterName = "";
	string workingString = "";
	while (i > -1) {
		if (fullName[i] == '-') {
			parameterName = workingString;
			workingString = "";
			i--;
			break;
		}
		workingString = fullName[i] + workingString;
		i--;
	}
	while (i > -1) {
		if (i < (int) fullName.size() - 1 && (fullName[i] == ':' && fullName[i + 1] == ':')) {
			category = workingString.substr(1, workingString.size());
			workingString = ":";
			break;
		} else {
			workingString = fullName[i] + workingString;
		}
		i--;
	}

	while (i > -1) {
		workingString = fullName[i] + workingString;
		i--;
	}

	if (category == "") {
		category = workingString;
	} else {
		nameSpace = workingString;
	}
}
*/
void Parameters::parseFullParameterName(const std::string &full_name,
                                        std::string &name_space_name,
                                        std::string &category_name,
                                        std::string &parameter_name) {
  // no need for error checking, since this is internal??
  // Then it should be a private member at least.

  std::regex param(R"(^(.*::)?(\w+)-(\w+)$)");
  std::smatch m;
  if (std::regex_match(full_name, m, param)) {
    name_space_name = m[1].str();
    category_name = m[2].str();
    parameter_name = m[3].str();
  } else {
    // this doesn't distinguish between command line parameters and setting file
    // parameters
    std::cout << "  ERROR! :: found misformatted parameter \"" << full_name
         << "\"\n  Parameters must have format: [category]-[name] "
            "or [name space][category]-[name]"
         << std::endl;
    exit(1);
  }
}

void Parameters::readCommandLine(
    int argc, const char **argv,
    std::unordered_map<std::string, std::string> &param_name_values, std::vector<std::string> &file_list,
    bool &save_files) {

  std::string usage_message =
      R"( [-f <file1> <file2> ...] [-p <parameter name/value pairs>] [-s]
                                    
  -f : "load files" - list of settings files to be loaded.
       Parameters in later files overwrite parameters in earlier files.

  -p : "set parameters" - list of parameter/name pairs. 
        e.g. "-p GLOBAL-updates 100 GLOBAL-popSize 200" would set MABE to 
        run for 100 updates with a population size of 200. Parameters set 
        on the command line overwrite parameters from files.

  -s : "save" - save settings files.

  -l : "create population loading script"
        This creates a default file "population_loader.plf" that contains 
        the script for loading the initial population. See file or wiki
        for usage examples. Note: using -l will ignore all other command 
        line arguments.

)";
   std::string default_plf_contents = R"(

MASTER = default 100 # by default :) 

# At the moment default and random mean the same thing. This will change
#MASTER = random 100

# an example loading a single file generated by default-archivist
#MASTER = 'snapshot_organisms_10.csv' 

# another example
#some_var = greatest 5 by ID from { '*.csv' }
#MASTER = collapse some_var
 
# an example with exact match
#MASTER = match ID where 3 from 'snapshot_organisms_0.csv' 

# a convoluted example :P
#another_var = greatest 5 by ID from { '*.csv' } 
#still_another_var = greatest 2 by ID from { '*.csv' : least 10 by score_AVE from { */LOD_*.csv : */SSWD_*.csv } : '*/*.csv' } 
#MASTER = collapse { any 3 from { another_var : still_another_var } }

)";

  std::string arguments;
  for (int i = 1; i < argc; i++)
    arguments += argv[i], arguments += " ";
  std::regex command_line_arguments(R"(-([a-z]) (.*?)(?=(?:(?:-[a-z] )|$)))");
  for (auto &m : forEachRegexMatch(arguments, command_line_arguments)) {
    switch (m[1].str()[0]) {
    case 'h':
      std::cout << "Usage: " << argv[0] << usage_message << std::endl;
      exit(1);
    case 'l': {
      std::ofstream plf_file("population_loader.plf");
      plf_file << default_plf_contents;
      plf_file.close();
      std::cout << "created population loader file "
                   "\"population_loader.plf\""
                << std::endl
                << "Change parameter GLOBAL-initPop to this file name (or "
                   "any other .plf file name) to load a specific population"
                << std::endl;
      exit(0);
    }
    case 's':
      save_files = true;
      break;
    case 'f': {
      std::stringstream sf(m[2].str());
      std::string filename;
      while (sf >> filename)
        file_list.push_back(filename);
      break;
    }
    case 'p': {
      std::stringstream sp(m[2].str());
      std::string param_name, param_value;
      while (sp >> param_name) {
        if (sp >> param_value) {
          if (param_name_values.find(param_name) != param_name_values.end()) {
            std::cout << "  ERROR :: Parameter \"" << param_name
                 << "\" is defined more then once on the command "
                    "line.\nExiting.\n";
            exit(1);
          }
          param_name_values[param_name] = param_value;
        } else {
          std::cout << "  ERROR :: Parameter \"" << param_name
               << "\" is defined on command line with out a "
                  "value.\nExiting.\n";
          exit(1);
        }
      }
      break;
    }
    default:
      std::cout << "  Error on command line. Unrecognized option. Exiting." << std::endl;
      exit(1);
    }
  }
} // end Parameters::readCommandLine()

/*
unordered_map<string, string> Parameters::readParametersFile(string fileName) {
	unordered_map<string, string> config_file_list;
	set<char> nameFirstLegalChars = {  // characters that can be used as the first "letter" of a name
			'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '_' };
	set<char> nameLegalChars = {  // characters that can be used in the body of a name
			'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '_' };
	set<char> whiteSpaceChars = { ' ', '\t' };

	string line;
	int line_number = 0;
	unsigned int index;
	string parameterName;
	string parameterValue;

	cout << "  - loading file \"" << fileName << "\"\n";

	ifstream configFile(fileName);  // open file named by fileName
	if (configFile.is_open())  // if the file named by fileName can be opened
	{
		string categoryName = "";
		string nameSpace = "";
		while (getline(configFile, line))  // keep loading one line from the file at a time into "line" until we get to the end of the file
		{
			line_number++;
			//cout << "line #: " << line_number << endl;
			index = 0;
			parameterValue = "";
			parameterName = "";
			while (index < line.length() && iswspace(line[index])) {
				index++;
			}

			if (index < line.length() && line[index] == '+') {  //if a line starts with % parse a catagory name
				string newNestingName = "";
				index++;
				if (line[index] != ' ') {
					cout << "  - ##### Config File: SYNTAX ERROR on line " << line_number << " in file: \"" << fileName << "\".\nExiting.\n";
					cout << "    While reading a nameSpace name, was expecting a blank space after \"+\".\n" << endl;
					exit(1);
				}
				index++;
				// get name (must start with letter)
				if (index < line.length() && isalpha(line[index])) {
					newNestingName += line[index];
					index++;
				} else if (index < line.length()) {  // if the not a name start character
					cout << "  - ##### Config File: SYNTAX ERROR on line " << line_number << " in file: \"" << fileName << "\".\nExiting.\n";
					cout << "    While reading a nameSpace name, found invalid first character after \"+\".\n" << endl;
					exit(1);
				}
				// get rest of name, must be numbers or letters (no '_'s) -- why no '_'?? changed this (see next line).
				while (index < line.length() && (isalnum(line[index]) || line[index]=='_' || line[index] == ':')) {
					newNestingName += line[index];
					index++;
				}
				while (index < line.length() && iswspace(line[index])) {
					index++;
				}
				if (index < line.length() && line[index] != '#') {
					cout << "  - ##### Config File: SYNTAX ERROR on line " << line_number << " in file: \"" << fileName << "\".\nExiting.\n";
					cout << "    While reading nameSpace name \"" << newNestingName << "\", found unused/invalid characters after nameSpace define outside of comment." << endl;
					exit(1);
				}
				if (newNestingName != "") {
					nameSpace = nameSpace + newNestingName;
					if (newNestingName[newNestingName.size() - 1] != ':' || newNestingName[newNestingName.size() - 2] != ':') {
						cout << "  - ##### Config File: SYNTAX ERROR on line " << line_number << " in file: \"" << fileName << "\".\n";
						cout << "    nameSpace part \"" << newNestingName << "\" (part of namespace \"" << nameSpace << "\") does not end in \"::\". NameSpace must end in \"::.\"\n    Exiting.\n" << endl;
						exit(1);
					}
					
						//nameSpace = nameSpace + newNestingName + "::"; // CODE CHANGE 8/28/2017 must end namespace with ::
				}
				index = line.length();  // move to end of line
			}

			if (index < line.length() && line[index] == '-') {  //if a line starts with - move up one name space
				if (nameSpace == "") {
					cout << "  - ##### Config File: SYNTAX ERROR on line " << line_number << " in file: \"" << fileName << "\". Exiting.\n";
					cout << "    attempt to leave nameSpace, when not in nameSpace" << endl;
					exit(1);
				} else {
					int cutPoint = nameSpace.size() - 2;
					while ((nameSpace[cutPoint] != ':' || nameSpace[cutPoint - 1] != ':') && cutPoint > 0) {
						//cout << cutPoint << " ";
						cutPoint--;
						//cout << cutPoint << endl;
					}
					//cout << "b: " << nameSpace << endl;
					if (cutPoint == 0) {
						nameSpace = "";
					} else {
						nameSpace = nameSpace.substr(0, cutPoint + 1);
					}
					//cout << "a: " << nameSpace << endl;
				}
				index = line.length();  // move to end of line
			}

			if (index < line.length() && line[index] == '%') {  //if a line starts with % parse a catagory name
				categoryName = "";
				index++;
				while (index < line.length() && iswspace(line[index])) {
					index++;
				}
				// get name (must start with letter or "_")
				if (index < line.length() && (isalpha(line[index]) || line[index] == '_')) {
					categoryName += line[index];
					index++;
				} else if (index < line.length()) {  // if the first character is not a letter or "_"
					cout << "  - ##### Config File: SYNTAX ERROR on line " << line_number << " in file: \"" << fileName << "\". Exiting.\n";
					cout << "    While category name found invalid character after \"%\"\n";
					exit(1);
				}
				// get rest of name, must be numbers or letters or '_'s
				while (index < line.length() && (isalnum(line[index]) || line[index] == '_')) {
					categoryName += line[index];
					//cout << newCategoryName << endl;
					index++;
				}
				while (index < line.length() && iswspace(line[index])) {
					index++;
				}
				if (index < line.length() && line[index] != '#') {
					cout << "  - ##### Config File: SYNTAX ERROR on line " << line_number << " in file: \"" << fileName << "\". Exiting.\n";
					cout << "    While reading category name \"" << categoryName << "\", found unused/invalid characters after category define outside of comment." << endl;
					exit(1);
				}
				index = line.length();  // move to end of line
			}

			// move past any leading white space
			while (index < line.length() && whiteSpaceChars.find(line[index]) != whiteSpaceChars.end()) {
				index++;
			}
			// comment can start before name, if it does, we are done with this line... move to end of line
			if (index < line.length() && line[index] == '#') {
				index = line.length();
			}
			// get name (must start with letter or "_")
			if (index < line.length() && nameFirstLegalChars.find(line[index]) != nameFirstLegalChars.end()) {
				parameterName += line[index];
				index++;
			} else if (index < line.length()) {  // if the first non whitespace character is not "#" or a name start character
				parameterName = "BAD_PARAMETER_NAME";  // set the paramterName to tell later error not to print
				index = line.length();
			}
			// get rest of name
			while (index < line.length() && ((nameLegalChars.find(line[index]) != nameLegalChars.end()) || (line[index] == '-') || (line[index] == ':'))) {
				parameterName += line[index];
				index++;
			}
			if (parameterName != "") {
				if (categoryName != "") {
					parameterName = categoryName + "-" + parameterName;
				}

				if (nameSpace != "") {
					parameterName = nameSpace + parameterName;
				}
			}
			//cout << parameterName << "   " << nameSpace << " " << categoryName << "\n";
			// move past white space between name and "="
			while (index < line.length() && whiteSpaceChars.find(line[index]) != whiteSpaceChars.end()) {
				index++;
			}
			// the next character must be "="
			if (index < line.length() && line[index] == '=') {
				index++;
				// move past white space between "=" and value
				while (index < line.length() && whiteSpaceChars.find(line[index]) != whiteSpaceChars.end()) {
					index++;
				}
				// get value : values can be made up of any characters
				while (index < line.length() && whiteSpaceChars.find(line[index]) == whiteSpaceChars.end() && line[index] != '#') {
					parameterValue += line[index];
					index++;
				}
				// move though whitespace till "#" or EoL
				while (index < line.length() && (whiteSpaceChars.find(line[index]) != whiteSpaceChars.end() || line[index] == '#')) {
					if (line[index] == '#') {  // if "#" move to end of line
						index = line.length();
					} else {  // move past whitespace
						index++;
					}
				}
			}
			if (index != line.length()) {  // if not at end of line, there was a problem
				cout << "  ERROR :: SYNTAX ERROR on line " << line_number << " in file: \"" << fileName << "\".\nExiting\n";
				exit(1);
			} else {
				// if there is a name and a value
				if (parameterName != "" && parameterValue != "") {
					if (config_file_list.find(parameterName) == config_file_list.end()) {
						config_file_list[parameterName] = string(parameterValue);
					} else {
						cout << "  - \"" << string(parameterName) << "\" is defined more then once in file: \"" << fileName << "\".\n exiting.\n";
						exit(1);
					}

				} else if (parameterName != "" && parameterValue == "") {  // if there is a name but no value, throw warning
					if (parameterName != "BAD_PARAMETER_NAME") {  // if "BAD_PARAMETER_NAME" then we already printed a warning.
						cout << "  ERROR :: SYNTAX ERROR on line " << line_number << " in file: \"" << fileName << "\".\nExiting.\n";
						exit(1);
					}
				}
			}
		}
		configFile.close();
	} else {
		cout << "  ERROR! unable to open file \"" << fileName << "\".\nExiting.\n";
		exit(1);
	}
	return config_file_list;
}
*/

std::unordered_map<std::string, std::string>
Parameters::readParametersFile(std::string file_name) {

  std::unordered_map<std::string, std::string> config_file_list;

  std::ifstream file(file_name); // open file named by file_name
  if (!file.is_open()) {
    std::cout << "  ERROR! unable to open file \"" << file_name << "\".\nExiting.\n";
    exit(1);
  }
  std::string dirty_line;
  std::string category_name;
  std::string name_space_name;

  while (std::getline(file, dirty_line)) {

    std::regex comments("#.*");
    std::string line = std::regex_replace(dirty_line, comments, "");

    std::regex empty_lines(R"(^\s*$)");
    if (std::regex_match(line, empty_lines))
      continue;

    {
      std::regex category(R"(^\s*%\s*(\w*)\s*$)");
      std::smatch m;
      if (std::regex_match(line, m, category)) {
        category_name = m[1].str();
		continue;
      }
    }

    {
      std::regex name_space_open(R"(^\s*\+\s*(\w+::)\s*$)");
      std::smatch m;
      if (std::regex_match(line, m, name_space_open)) {
        name_space_name += m[1].str();
        continue;
      }
    }

    {
      std::regex name_space_close(R"(^\s*-\s*$)");
      std::regex name_space_remove_last(R"(::\w+::)");
      std::smatch m;
      if (std::regex_match(line, m, name_space_close)) {
        if (name_space_name.empty()) {
          std::cout << " Error: no namespace to descend, already at root:: "
                  "namespace. "
               << std::endl;
          exit(1);
        }
        name_space_name =
            std::regex_replace(name_space_name, name_space_remove_last, "::");
        continue;
      }
	}

    {
      std::regex name_value_pair(R"(^\s*(\w+)\s*=\s*(\S?.*\S)\s*$)");
      std::smatch m;
      if (std::regex_match(line, m, name_value_pair)) {
        auto name = name_space_name + category_name + "-" + m[1].str();
        if (config_file_list.find(name) != config_file_list.end()) {
          std::cout << "  Error: \"" << name << "\" is defined more then once in file: \""
               << file_name << "\".\n exiting.\n";
          exit(1);
        }
        config_file_list[name] = m[2].str();
        continue;
      }
    }

    std::cout
        << " Error: unrecognised line " << std::endl
        << dirty_line << " in file " << file_name << std::endl
        << R"(See https://github.com/Hintzelab/MABE/wiki/Parameters-Name-Space  for correct usage.)"
        << std::endl;
  }

  return config_file_list;
}

/*
bool Parameters::initializeParameters(int argc, const char * argv[]) {

	if (root == nullptr) {
		root = ParametersTable::makeTable();
	}

	unordered_map<string, string> command_line_list;
	vector<string> fileList;

	bool saveFiles = false;
	Parameters::readCommandLine(argc, argv, command_line_list, fileList, saveFiles);

	string workingNameSpace, workingCategory, workingParameterName;

	for (auto fileName : fileList) {  // load all files in order
		unordered_map<string, string> file_list = Parameters::readParametersFile(fileName);
		for (auto i : file_list) {
			parseFullParameterName(i.first, workingNameSpace, workingCategory, workingParameterName);
			if (workingParameterName == ""){
				//ASSERT(workingParameterName != "", "  ERROR! :: reading from file \"" << fileName << "\" found misformatted parameter \"" << i.first << "\"\n  Parameters must have format: [category]-[name] or [name space][category]-[name]");
				cout << "  ERROR! :: reading from file \"" << fileName << "\" found misformatted parameter \"" << i.first << "\"\n  Parameters must have format: [category]-[name] or [name space][category]-[name]" << endl;
				exit(1);
			}
			string parameterType = root->getParameterType(workingCategory + "-" + workingParameterName);
			if (parameterType == "bool") {
				bool value;
				stringToValue(i.second, value);
				root->setParameter(workingCategory + "-" + workingParameterName, value, workingNameSpace, true);
			} else if (parameterType == "string") {
				string value;
				stringToValue(i.second, value);
				root->setParameter(workingCategory + "-" + workingParameterName, value, workingNameSpace, true);
			} else if (parameterType == "int") {
				int value;
				stringToValue(i.second, value);
				root->setParameter(workingCategory + "-" + workingParameterName, value, workingNameSpace, true);
			} else if (parameterType == "double") {
				double value;
				stringToValue(i.second, value);
				root->setParameter(workingCategory + "-" + workingParameterName, value, workingNameSpace, true);
			} else {
				if (saveFiles) {
					cout << "   WARRNING";
				} else {
					cout << "  ERROR";
				}
				cout << " :: while reading file \"" << fileName << "\" found \"" << workingNameSpace + workingCategory + "-" + workingParameterName << ".\n      But \"" << workingCategory + "-" + workingParameterName << "\" is not a registered parameter!" << endl;
				if (saveFiles) {
					cout << "      This parameter will not be saved to new files." << endl;
				} else {
					cout << "  Exiting." << endl;
					exit(1);
				}
			}
		}
	}
	for (auto i : command_line_list) {  // load command line parameters last
		parseFullParameterName(i.first, workingNameSpace, workingCategory, workingParameterName);
		if (workingParameterName == ""){
			//ASSERT(workingParameterName != "", "  ERROR! :: reading from command line found misformatted parameter \"" << i.first << "\"\n  Parameters must have format: [category]-[name] or [name space][category]-[name]");
			cout << "  ERROR! :: reading from command line found misformatted parameter \"" << i.first << "\"\n  Parameters must have format: [category]-[name] or [name space][category]-[name]" << endl;
			exit(1);
		}
		string parameterType = root->getParameterType(workingCategory + "-" + workingParameterName);
		if (parameterType == "bool") {
			bool value;
			stringToValue(i.second, value);
			root->setParameter(workingCategory + "-" + workingParameterName, value, workingNameSpace, true);
		} else if (parameterType == "string") {
			string value;
			stringToValue(i.second, value);
			root->setParameter(workingCategory + "-" + workingParameterName, value, workingNameSpace, true);
		} else if (parameterType == "int") {
			int value;
			stringToValue(i.second, value);
			root->setParameter(workingCategory + "-" + workingParameterName, value, workingNameSpace, true);
		} else if (parameterType == "double") {
			double value;
			stringToValue(i.second, value);
			root->setParameter(workingCategory + "-" + workingParameterName, value, workingNameSpace, true);
		} else {
			if (saveFiles) {
				cout << "   WARRNING";
			} else {
				cout << "  ERROR";
			}
			cout << " :: while reading command line found \"" << workingNameSpace + workingCategory + "-" + workingParameterName << ".\n      But \"" << workingCategory + "-" + workingParameterName << "\" is not a registered parameter!" << endl;
			if (saveFiles) {
				cout << "      This parameter will not be saved to new files." << endl;
			} else {
				cout << "  Exiting." << endl;
				exit(1);
			}
		}
	}
	return saveFiles;
//	if (saveFiles) {
//
//		Parameters::saveSettingsFiles(_maxLineLength, _commentIndent, { "*" }, { { "settings_organism.cfg", { "GATE*", "GENOME*", "BRAIN*" } }, { "settings_world.cfg", { "WORLD*" } }, { "settings.cfg", { "" } } });
//		cout << "Saving config Files and Exiting." << endl;
//		exit(0);
//	}
}
*/

bool Parameters::initializeParameters(int argc, const char *argv[]) {

  if (root == nullptr) {
    root = ParametersTable::makeTable();
  }

  std::unordered_map<std::string, std::string> command_line_list;
  std::vector<std::string> fileList;

  bool saveFiles = false;
  Parameters::readCommandLine(argc, argv, command_line_list, fileList,
                              saveFiles);

  std::string workingNameSpace, workingCategory, workingParameterName;

  for (const auto &fileName : fileList) { // load all files in order - this
                                          // order is arbitrary if wildcarded?
    std::unordered_map<std::string, std::string> file_list =
        Parameters::readParametersFile(fileName);
    for (const auto &file : file_list) {
      parseFullParameterName(file.first, workingNameSpace, workingCategory,
                             workingParameterName);
      if (!root->getParameterTypeAndSetParameter(
              workingCategory + "-" + workingParameterName, file.second,
              workingNameSpace, true)) {
        std::cout << (saveFiles ? "   WARNING" : "  ERROR")
             << " :: while reading file \"" << fileName << "\" found \""
             << workingNameSpace + workingCategory + "-" + workingParameterName
             << ".\n      But \""
             << workingCategory + "-" + workingParameterName
             << "\" is not a registered parameter!" << std::endl
             << (saveFiles ? "      This parameter will not be saved "
                             "to new files."
                           : "  Exiting.")
             << std::endl;
        if (!saveFiles) {
          exit(1);
        }
      }
    }
  }
  for (const auto &command :
       command_line_list) { // load command line parameters last
    parseFullParameterName(command.first, workingNameSpace, workingCategory,
                           workingParameterName);
    if (!root->getParameterTypeAndSetParameter(
            workingCategory + "-" + workingParameterName, command.second,
            workingNameSpace, true)) {
      std::cout << (saveFiles ? "   WARNING" : "  ERROR")
           << " :: while reading command line found \""
           << workingNameSpace + workingCategory + "-" + workingParameterName
           << ".\n      But \"" << workingCategory + "-" + workingParameterName
           << "\" is not a registered parameter!" << std::endl
           << (saveFiles ? "      This parameter will not be saved "
                           "to new files."
                         : "  Exiting.")
           << std::endl;
      if (!saveFiles) {
        exit(1);
      }
    }
  }
  return saveFiles;
}

/* **** old version will be deprecated upon resolution of MABE namespace
semantics
void Parameters::saveSettingsFile(const string& nameSpace, stringstream& FILE,
vector<string> categoryList, int _maxLineLength, int _commentIndent, bool
alsoChildren, int nameSpaceLevel) {
        map<string, vector<string>> sortedParameters;
        root->lookupTable(nameSpace)->parametersToSortedList(sortedParameters);
        if (!root->lookupTable(nameSpace)->neverSave) {
                string currentIndent = "";
                vector<string> nameSpaceParts = nameSpaceToNameParts(nameSpace);

                for (int i = 0; i < nameSpaceLevel; i++) {
                        currentIndent += "  ";
                        nameSpaceParts.erase(nameSpaceParts.begin());
                }

                if (nameSpaceParts.size() > 0) {
                        for (auto p : nameSpaceParts) {
                                FILE << currentIndent << "+ " << p.substr(0,
p.size() - 2) << "\n";
                                nameSpaceLevel++;
                                currentIndent += "  ";
                        }
                }
                if (categoryList.size() > 0 && categoryList[0] == "-") {
                        if (sortedParameters.find("GLOBAL") !=
sortedParameters.end() && !(find(categoryList.begin(), categoryList.end(),
"GLOBAL") != categoryList.end())) {
                                FILE << currentIndent << "% GLOBAL" << "\n";
                                for (auto parameter :
sortedParameters["GLOBAL"]) {
                                        printParameterWithWraparound(FILE,
currentIndent + "  ", parameter, _maxLineLength, _commentIndent);
//					FILE << currentIndent << "  " <<
parameter << "\n";
                                }
                                FILE << "\n";
                        }
                } else {  // write parameters to file.
                        if (sortedParameters.find("GLOBAL") !=
sortedParameters.end() && find(categoryList.begin(), categoryList.end(),
"GLOBAL") != categoryList.end()) {
                                FILE << currentIndent << "% GLOBAL" << "\n";
                                for (auto parameter :
sortedParameters["GLOBAL"]) {
                                        printParameterWithWraparound(FILE,
currentIndent + "  ", parameter, _maxLineLength, _commentIndent);
//					FILE << currentIndent << "  " <<
parameter << "\n";
                                }
                                FILE << "\n";
                        }

                }
                sortedParameters.erase("GLOBAL");

                for (auto group : sortedParameters) {
                        bool saveThis = false;
                        if (categoryList.size() > 0 && categoryList[0] != "-") {
                                for (auto cat : categoryList) {
                                        if ((int) group.first.size() >= ((int)
cat.size()) - 1) {
                                                if (group.first == cat) {
                                                        saveThis = true;
                                                } else {
                                                        if ((int) cat.size() > 0
&& cat[((int) cat.size()) - 1] == '*') {
                                                                if
(group.first.substr(0, cat.size() - 1) == cat.substr(0, cat.size() - 1)) {
                                                                        saveThis
= true;
                                                                }
                                                        }
                                                }
                                        }

                                }
                        } else {
                                saveThis = true;
                                for (auto cat : categoryList) {
                                        if ((int) group.first.size() >= ((int)
cat.size()) - 1) {
                                                if (group.first == cat) {
                                                        saveThis = false;
                                                } else {
                                                        if ((int) cat.size() > 0
&& cat[((int) cat.size()) - 1] == '*') {
                                                                if
(group.first.substr(0, cat.size() - 1) == cat.substr(0, cat.size() - 1)) {
                                                                        saveThis
= false;
                                                                }
                                                        }
                                                }
                                        }
                                }
                        }
                        if (saveThis) {
                                FILE << currentIndent << "% " << group.first <<
"\n";
                                for (auto parameter : group.second) {
                                        printParameterWithWraparound(FILE,
currentIndent + "  ", parameter, _maxLineLength, _commentIndent);
//					FILE << currentIndent << "  " <<
parameter << "\n";
                                }
                                FILE << "\n";
                        }
                }
                if (alsoChildren) {
                        vector<shared_ptr<ParametersTable>> checklist =
root->lookupTable(nameSpace)->getChildren();
                        sort(checklist.begin(), checklist.end());
                        for (auto c : checklist) {
                                saveSettingsFile(c->getTableNameSpace(), FILE,
categoryList, _maxLineLength, _commentIndent, true, nameSpaceLevel);
                        }
                }

                while (nameSpaceParts.size() > 0) {
                        currentIndent = currentIndent.substr(2,
currentIndent.size());
                        FILE << currentIndent << "- (" <<
nameSpaceParts[nameSpaceParts.size() - 1].substr(0,
nameSpaceParts[nameSpaceParts.size() - 1].size() - 2) << ")\n";
                        nameSpaceParts.pop_back();
                }
                //cout << "  - \"" << fileName << "\" has been created.\n";
        }
}
*/

void Parameters::saveSettingsFile(const std::string &name_space,
                                  std::stringstream &file,
                                  std::vector<std::string> category_list,
                                  int max_line_length, int comment_indent,
                                  bool also_children, int name_space_level) {

  if (root->lookupTable(name_space)->neverSave)
    return;



  std::map<std::string, std::vector<std::string>> sortedParameters;
  root->lookupTable(name_space)->parametersToSortedList(sortedParameters);
    
  std::string current_indent = "";

/*   *** Will uncomment and fix when namespaces are actually needed
    auto name_space_parts = nameSpaceToNameParts(name_space);
	for (int i = 0; i < name_space_level; i++) {
      currentIndent += "  ";
      name_space_parts.erase(name_space_parts.begin());
    }

    if (name_space_parts.size() > 0) {
      for (auto p : name_space_parts) {
        file << currentIndent << "+ " << p.substr(0, p.size() - 2) << "\n";
        name_space_level++;
        currentIndent += "  ";
      }
    }
 */
 	if (category_list.size() > 0 && category_list[0] == "-") {
      if (sortedParameters.find("GLOBAL") != sortedParameters.end() &&
          !(find(category_list.begin(), category_list.end(), "GLOBAL") !=
            category_list.end())) {
        file << current_indent << "% GLOBAL"
             << "\n";
        for (auto parameter : sortedParameters["GLOBAL"]) {
          printParameterWithWraparound(file, current_indent + "  ", parameter,
                                       max_line_length, comment_indent);
          //					file <<
          // currentIndent << "  " << parameter << "\n";
        }
        file << "\n";
      }
    } else { // write parameters to file.
      if (sortedParameters.find("GLOBAL") != sortedParameters.end() &&
          find(category_list.begin(), category_list.end(), "GLOBAL") !=
              category_list.end()) {
        file << current_indent << "% GLOBAL"
             << "\n";
        for (auto parameter : sortedParameters["GLOBAL"]) {
          printParameterWithWraparound(file, current_indent + "  ", parameter,
                                       max_line_length, comment_indent);
          //					file <<
          // currentIndent << "  " << parameter << "\n";
        }
        file << "\n";
      }
    }
    sortedParameters.erase("GLOBAL");

    for (auto group : sortedParameters) {
      bool saveThis = false;
      if (category_list.size() > 0 && category_list[0] != "-") {
        for (auto cat : category_list) {
          if ((int)group.first.size() >= ((int)cat.size()) - 1) {
            if (group.first == cat) {
              saveThis = true;
            } else {
              if ((int)cat.size() > 0 && cat[((int)cat.size()) - 1] == '*') {
                if (group.first.substr(0, cat.size() - 1) ==
                    cat.substr(0, cat.size() - 1)) {
                  saveThis = true;
                }
              }
            }
          }
        }
      } else {
        saveThis = true;
        for (auto cat : category_list) {
          if ((int)group.first.size() >= ((int)cat.size()) - 1) {
            if (group.first == cat) {
              saveThis = false;
            } else {
              if ((int)cat.size() > 0 && cat[((int)cat.size()) - 1] == '*') {
                if (group.first.substr(0, cat.size() - 1) ==
                    cat.substr(0, cat.size() - 1)) {
                  saveThis = false;
                }
              }
            }
          }
        }
      }
      if (saveThis) {
        file << current_indent << "% " << group.first << "\n";
        for (auto parameter : group.second) {
          printParameterWithWraparound(file, current_indent + "  ", parameter,
                                       max_line_length, comment_indent);
          //					file << currentIndent << "  " << parameter <<
          //"\n";
        }
        file << "\n";
      }
    }

	if (also_children) {
      std::vector<std::shared_ptr<ParametersTable>> checklist =
          root->lookupTable(name_space)->getChildren();
      sort(checklist.begin(), checklist.end());
      for (auto c : checklist) {
        saveSettingsFile(c->getTableNameSpace(), file, category_list,
                         max_line_length, comment_indent, true, name_space_level);
      }
    }
/*   *** Will uncomment and fix when namespaces are actually needed
    while (name_space_parts.size() > 0) {
      currentIndent = currentIndent.substr(2, currentIndent.size());
      file << currentIndent << "- ("
           << name_space_parts[name_space_parts.size() - 1].substr(
                  0, name_space_parts[name_space_parts.size() - 1].size() - 2)
           << ")\n";
      name_space_parts.pop_back();
    }
*/    // cout << "  - \"" << fileName << "\" has been created.\n";
}

/*
void Parameters::printParameterWithWraparound(stringstream& FILE, string _currentIndent, string _parameter, int _maxLineLength, int _commentIndent) {
	int currentLineLength = _currentIndent.size();
	FILE << _currentIndent;
	int beforeIndentLength = currentLineLength + _parameter.find("@@@#");
	string betweenString = "";
	for (int i = beforeIndentLength; i < _commentIndent; i++) {
		betweenString.append(" ");
	}
	betweenString.append("#");
	_parameter.replace(_parameter.find("@@@#"), 4, betweenString);
	if (currentLineLength + (int) _parameter.size() < _maxLineLength && (int)_parameter.find("\n") == -1) {
		FILE << _parameter << endl;
	} else {
		string indent = "";
		for (int i = 0; i < _commentIndent; i++) {
			indent.append(" ");
		}
		indent.append("#    ");

		string parameterName = _parameter.substr(0, _parameter.find("#"));
		string parameterRemainder = _parameter.substr(_parameter.find("#"));
		FILE << parameterName;
		currentLineLength += parameterName.size();
		int remainderMaxLength = _maxLineLength - _commentIndent;
		while (parameterRemainder.size() > 0) {
			int newLine = parameterRemainder.substr(0, remainderMaxLength).find("\n");
			if (newLine == -1) {
				int lastSpace = parameterRemainder.substr(0, remainderMaxLength).find_last_of(" ");
				if (lastSpace == -1) {
					if (currentLineLength == 0) {
						FILE << indent;
						currentLineLength += _commentIndent;
					}
					if ((int) parameterRemainder.size() > remainderMaxLength) {
						string takenPart = parameterRemainder.substr(0, remainderMaxLength - 1);
						parameterRemainder = parameterRemainder.substr(remainderMaxLength - 1);
						takenPart.append("-");
						FILE << takenPart << endl;
					} else {
						FILE << parameterRemainder << endl;
						parameterRemainder = "";
					}
				} else {
					if (currentLineLength == 0) {
						FILE << indent;
						currentLineLength += _commentIndent;
					}
					if ((int) parameterRemainder.size() > remainderMaxLength) {
						string remainderBeforeSpace = parameterRemainder.substr(0, lastSpace);
						parameterRemainder = parameterRemainder.substr(lastSpace + 1);
						FILE << remainderBeforeSpace << endl;
					} else {
						FILE << parameterRemainder << endl;
						parameterRemainder = "";
					}
				}
			} else {
				if (currentLineLength == 0) {
					FILE << indent;
					currentLineLength += _commentIndent;
				}
				string remainderBeforeNewLine = parameterRemainder.substr(0, newLine);
				parameterRemainder = parameterRemainder.substr(newLine + 1);
				FILE << remainderBeforeNewLine << endl;
			}
			currentLineLength = 0;
		}
	}
}
*/

void Parameters::printParameterWithWraparound(std::stringstream &file,
                                              std::string current_indent,
                                              std::string entire_parameter,
                                              int max_line_length,
                                              int comment_indent) {

  auto pos_of_comment =
      entire_parameter.find_first_of("@@@#"); // must be cleaned
  if (pos_of_comment == std::string::npos) {
    std::cout << " Error : parameter has no comment";
    exit(1); // which makes type conversion to int safe after this??
  }
  if (int(pos_of_comment) > max_line_length - 9) {
    std::cout << " Warning: parameter name and value too large to fit on single "
            "line. Ignoring column width for this line\n";
  }

  std::string line;
  line += current_indent;
  line += entire_parameter.substr(0, pos_of_comment);  // write name-value

  std::string sub_line(comment_indent, ' ');
  if (int(line.length()) < comment_indent)
    line +=
        sub_line.substr(0, comment_indent - line.length()); // pad with spaces

  auto comment = entire_parameter.substr(pos_of_comment + 3); // + 3 must be cleaned

  // add as much of the comment as possible to the line
  auto next_newline = comment.find_first_of('\n');
  auto comment_cut = std::min(  // yuck, must express more clearly
      next_newline == std::string::npos ? max_line_length : int(next_newline),
      max_line_length - std::max(comment_indent, int(line.length())));
  line += comment.substr(0, comment_cut);
  file << line << '\n';
 
  // write rest of the comments right-aligned
  comment = comment.substr(std::min(comment_cut , int(comment.length())));
  std::regex aligned_comments(R"((.*\n|.{1,)" + std::to_string(max_line_length - comment_indent) +
                              R"(}))");
  for (auto &m : forEachRegexMatch(comment, aligned_comments)) {
    auto comment_piece = m[1].str();
    file << sub_line << "# " << comment_piece
         << (comment_piece.back() == '\n' ? "" : "\n");
  }
  file << '\n';
}

/*  **** old version will be deprecated upon resolution of MABE namespace semantics
void Parameters::saveSettingsFiles(int _maxLineLength, int _commentIndent, vector<string> nameSpaceList, vector<pair<string, vector<string>>> categoryLists) {
	bool alsoChildren;
	string fileName;
	vector<string> otherCategoryList;
	for (auto nameSpace : nameSpaceList) {
		for (auto cList : categoryLists) {
			otherCategoryList.insert(otherCategoryList.end(), cList.second.begin(), cList.second.end());
			if ((int)nameSpace.size() > 0 && nameSpace[nameSpace.size()-1] == '*') {
				nameSpace.pop_back();
				alsoChildren = true;
			} else {
				alsoChildren = false;
			}
			fileName = "";
			bool lastCharWasCol = false;
			for (auto c : nameSpace) {
				if (c != ':') {
					fileName += c;
					lastCharWasCol = false;
				} else {
					if (lastCharWasCol == true) {
						fileName += '_';
					}
					lastCharWasCol = true;
				}
			}
			if (fileName != "") {
				fileName = fileName.substr(0,fileName.size()-1)+"-";
			}
			stringstream ss;
			if (cList.second.size() == 1 && cList.second[0]=="") {
				otherCategoryList.insert(otherCategoryList.begin(),"-");
				saveSettingsFile(nameSpace, ss, otherCategoryList, _maxLineLength, _commentIndent, alsoChildren);
			} else {
				saveSettingsFile(nameSpace, ss, cList.second, _maxLineLength, _commentIndent, alsoChildren);
			}
			string workingString = ss.str();
			workingString.erase (remove (workingString.begin(), workingString.end(), ' '), workingString.end());
			workingString.erase (remove (workingString.begin(), workingString.end(), 11), workingString.end());
			bool lastCharEnter = false;
			bool fileEmpty = true;
			for (auto c : workingString) {
				if (c == 10) {
					lastCharEnter = true;
				} else {
					if (lastCharEnter == true) {
						if (!(c == '+' || c == '-' || c == 10)) {
							fileEmpty = false;
						}
					}
					lastCharEnter = false;
				}
			}
			if (!fileEmpty) {
				ofstream FILE(fileName+cList.first);
				FILE << ss.str();
				FILE.close();
			}
		}
	}
}
*/

void Parameters::saveSettingsFiles(
    int max_line_length, int comment_indent,
    std::vector<std::string> name_space_list,
    std::vector<std::pair<std::string, std::vector<std::string>>>
        category_lists) {
  bool also_children;
  std::string file_name;
  std::vector<std::string> other_category_list;
  for (auto name_space : name_space_list) {
    for (auto clist : category_lists) {
      other_category_list.insert(other_category_list.end(), clist.second.begin(),
                               clist.second.end());
      if (!name_space.empty() && name_space.back() == '*') {
        name_space.pop_back();
        also_children = true;
      } else {
        also_children = false;
      }
      // why bother at all?
      std::regex colon(R"(::)");
      file_name = std::regex_replace(name_space, colon, "_");
      if (!file_name.empty()) {
        file_name.pop_back();
        file_name += "_";
      }

      std::stringstream ss;
      if (clist.second.size() == 1 && clist.second[0] == "") {
        other_category_list.insert(other_category_list.begin(), "-");
        saveSettingsFile(name_space, ss, other_category_list, max_line_length,
                         comment_indent, also_children);
      } else {
        saveSettingsFile(name_space, ss, clist.second, max_line_length,
                         comment_indent, also_children);
      }
      std::string workingString = ss.str();
      workingString.erase(
          std::remove_if(workingString.begin(), workingString.end(),
                         [](char c) { return c == ' ' || c == 11; }),
          workingString.end());
      bool lastCharEnter = false;
      bool fileEmpty = true;
      for (auto c : workingString) {
        if (c == 10) {
          lastCharEnter = true;
        } else {
          if (lastCharEnter == true) {
            if (!(c == '+' || c == '-' || c == 10)) {
              fileEmpty = false;
            }
          }
          lastCharEnter = false;
        }
      }
      if (!fileEmpty) {
        std::ofstream file (file_name + clist.first);
        file << ss.str();
        file.close();
      }
    }
  }
} // end Parameters::saveSettingsFiles
