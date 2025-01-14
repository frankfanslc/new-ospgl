#pragma once
#include <cpptoml.h>
#include <glm/glm.hpp>
#include "Logger.h"

// REMEMBER TO INCLUDE ANY SERIALIZE YOU USE, OTHERWISE THE AUTOMATIC
// SERIALIZATION FUNCTIONS WON'T SEE THE APPROPIATE TEMPLATE CLASSES

#define TOML_CHECK_FUNC logger->check

// IMPORTANT:
// You must have a cpptoml table named "from" to use these functions

#define SAFE_TOML_GET_FROM(from, target, name, type) \
	TOML_CHECK_FUNC((from).get_qualified_as<type>(name).operator bool(), "Data " name " of type " #type " was malformed ({})", ::_serializer_path); \
	target = *(from).get_qualified_as<type>(name);

#define SAFE_TOML_GET(target, name, type) \
	TOML_CHECK_FUNC(from.get_qualified_as<type>(name).operator bool(), "Data " name " of type " #type " was malformed ({})", ::_serializer_path); \
	target = *from.get_qualified_as<type>(name);

#define SAFE_TOML_GET_OR(target, name, type, def) \
	target = from.get_qualified_as<type>(name).value_or(def);

#define SAFE_TOML_GET_TABLE(target, name, type) \
	TOML_CHECK_FUNC(from.get_table_qualified(name).operator bool(), "Table " name " of type" #type " was malformed ({})", ::_serializer_path); \
	{auto table = from.get_table_qualified(name); ::deserialize<type>(target, *table);}

#define SAFE_TOML_GET_OR_IGNORE(target, name, type) \
	do{ auto t = from.get_qualified_as<type>(name); if (t){target = *t;}}while(0);

#define NOT_SERIALIZABLE(type) static void serialize(const type &, cpptoml::table& target) {\
	logger->check(false, "Serialize called on non-serializable type: " #type); }


thread_local static std::string _serializer_path;

template<typename T>
class GenericSerializer
{
public:


	static void serialize(const T& what, cpptoml::table& target)
	{
		logger->fatal("Default serializer called. Remember to include the serializers you use!");
	}

	static void deserialize(T& to, const cpptoml::table& from)
	{
		logger->fatal("Default deserializer called. Remember to include the serializers you use!");
	}
};

template<typename T> 
void serialize(const T& what, cpptoml::table& target)
{
	GenericSerializer<T>::serialize(what, target);
}

template<typename T>
void deserialize(T& to, const cpptoml::table& from)
{
	GenericSerializer<T>::deserialize(to, from);
}

template<typename T>
void serialize_to_table(const T& what, cpptoml::table& root, const std::string& table_name)
{
	auto table = cpptoml::make_table();
	serialize(what, *table);
	root.insert(table_name, table);
}

class SerializeUtil
{
public:

	static void write_to_file(const cpptoml::table& table, const std::string& path)
	{
		std::ofstream file(path);
		file << table;
	}

	static const std::shared_ptr<cpptoml::table> load_file(const std::string& path)
	{
		try
		{
			return cpptoml::parse_file(path);
		}
		catch (const cpptoml::parse_exception& ex)
		{
			logger->fatal("TOML parse error:\n{}", ex.what());
			return nullptr;
		}
	}

	static const std::shared_ptr<cpptoml::table> load_string(const std::string& str)
	{
		std::stringstream ss = std::stringstream(str);
		try
		{
			return cpptoml::parser(ss).parse();
		}
		catch (const cpptoml::parse_exception& ex)
		{
			logger->fatal("TOML parse error:\n{}", ex.what());
			return nullptr;
		}
	}

	static const std::shared_ptr<cpptoml::table> load_or_write_default(const cpptoml::table& def, const std::string& path)
	{
		std::ifstream infile(path);
		bool read = infile.good();
		if (read)
		{
			return load_file(path);

		}
		else
		{
			write_to_file(def, path);
			// Now it will be able to read it
			return load_file(path);
		}
	}

	template<typename T>
	static void read_to(const cpptoml::table& root, T& target, const std::string& sub_path = "")
	{;
		if (sub_path != "")
		{
			deserialize(target, *root.get_table(sub_path));
		}
		else
		{
			deserialize(target, root);
		}


	}

	// Leave subpath empty for root
	template<typename T>
	static void read_file_to(const std::string& path, T& target, const std::string& sub_path = "")
	{
		auto root = load_file(path);

		_serializer_path = path;

		std::shared_ptr<cpptoml::table> from;

		if (sub_path != "")
		{
			from = root->get_table(sub_path);
		}
		else
		{
			from = root;
		}

		deserialize(target, *from);

		_serializer_path = "memory";
	}

	// Optionally allows only overwriting a part of a file if sub_path != ""
	template<typename T>
	static void write_to(const std::string path, const T& source, const std::string& sub_path = "")
	{
		_serializer_path = path;

		auto serialized = cpptoml::make_table();
		serialize(source, *serialized);

		if (sub_path == "")
		{
			write_to_file(*serialized, path);
		}
		else
		{
			// We need to load the file and change only what's needed
			auto root = load_file(path);

			if (root->get_table(sub_path))
			{
				root->erase(sub_path);
			}

			root->insert(sub_path, serialized);
			
			write_to_file(*root, path);
		}

		_serializer_path = "memory";
	}

	// Overrides target with source, overwriting any entry when needed
	static void override(cpptoml::table& target, cpptoml::table& source)
	{
		for(auto children : source)
		{
			if(target.contains(children.first))
			{
				if(children.second->is_table())
				{
					// Recurse
					override(*target.get_table(children.first), *children.second->as_table());	
				}
				else if(children.second->is_table_array())
				{
					logger->fatal("Not implemented TODO"); 
					// TODO: We can either insert the new items or overwrite the whole thing
				}
				else if(children.second->is_array())
				{
					// Overwrite value (the whole array)
					target.erase(children.first);
					target.insert(children.first, children.second);
				}
				else if(children.second->is_value())
				{
					// Overwrite value
					target.erase(children.first);
					target.insert(children.first, children.second);
				}
			}
			else
			{
				// Not present in target table, simply insert it
				target.insert(children.first, children.second);
			}
		}
	}
};
