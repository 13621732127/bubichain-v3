#include <utils/utils.h>
#include <utils/file.h>
#include <utils/strings.h>
#include <utils/logger.h>
#include <utils/crypto.h>
#include "cert_configure.h"
#include <utils/net.h>
namespace bumo {

	CertDbConfigure::CertDbConfigure() {
		cert_db_path_ = "data/cert.db";
		tmp_path_ = "tmp";
		async_write_sql_ = false; //default sync write sql
		async_write_kv_ = false; //default sync write kv
	}

	CertDbConfigure::~CertDbConfigure() {}

	bool CertDbConfigure::Load(const Json::Value &value) {
		ConfigureBase::GetValue(value, "cert_db_path", cert_db_path_);
		ConfigureBase::GetValue(value, "rational_string", rational_string_);
		ConfigureBase::GetValue(value, "rational_db_type", rational_db_type_);
		ConfigureBase::GetValue(value, "tmp_path", tmp_path_);
		ConfigureBase::GetValue(value, "async_write_sql", async_write_sql_);
		ConfigureBase::GetValue(value, "async_write_kv", async_write_kv_);


		std::string rational_decode;
		std::vector<std::string> nparas = utils::String::split(rational_string_, " ");
		for (std::size_t i = 0; i < nparas.size(); i++) {
			std::string str = nparas[i];
			std::vector<std::string> n = utils::String::split(str, "=");

			if (n.size() == 2 && n[0] == "password") {
				n[1] = utils::Aes::HexDecrypto(n[1], GetDataSecuretKey());
				str = utils::String::Format("%s=%s", n[0].c_str(), n[1].c_str());
			}
			rational_decode += " ";
			rational_decode += str;
		}
		rational_string_ = rational_decode;

		if (!utils::File::IsAbsolute(cert_db_path_)) {
			cert_db_path_ = utils::String::Format("%s/%s", utils::File::GetBinHome().c_str(), cert_db_path_.c_str());
		}

		if (!utils::File::IsAbsolute(tmp_path_)) {
			tmp_path_ = utils::String::Format("%s/%s", utils::File::GetBinHome().c_str(), tmp_path_.c_str());
		}
		return true;
	}

	CertServerConfigure::CertServerConfigure() {
		ssl_enable_ = false;
		query_limit_ = 1000;
		multiquery_limit_ = 100;
		thread_count_ = 0;
	}

	CertServerConfigure::~CertServerConfigure() {}

	bool CertServerConfigure::Load(const Json::Value &value) {
		std::string listen_address_value;
		ConfigureBase::GetValue(value, "listen_addresses", listen_address_value);
		utils::StringVector address_array = utils::String::Strtok(listen_address_value, ',');
		for (size_t i = 0; i < address_array.size(); i++) {
			listen_addresses_.push_back(utils::InetAddress(address_array[i]));
		}
		ConfigureBase::GetValue(value, "index_name", index_name_);
		ConfigureBase::GetValue(value, "directory", directory_);
		ConfigureBase::GetValue(value, "ssl_enable", ssl_enable_);
		ConfigureBase::GetValue(value, "query_limit", query_limit_);
		ConfigureBase::GetValue(value, "multiquery_limit", multiquery_limit_);
		ConfigureBase::GetValue(value, "thread_count", thread_count_);
		ConfigureBase::GetValue(value, "allow_origin", allow_origin_);
		
		
		if (ssl_enable_)
			ssl_configure_.Load(value["ssl"]);
		return true;
	}

	CertConfigure::CertConfigure() {}

	CertConfigure::~CertConfigure() {}

	bool CertConfigure::LoadFromJson(const Json::Value &values){
		if (!values.isMember("db") ||
			!values.isMember("logger")) {
			LOG_STD_ERR("Some configuration not exist");
			return false;
		}

		cert_db_configure_.Load(values["db"]);
		logger_configure_.Load(values["logger"]);
		webserver_configure_.Load(values["webserver"]);
		return true;
	}
}