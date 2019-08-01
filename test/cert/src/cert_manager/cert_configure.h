#ifndef CONFIGURE_CERT_H_
#define CONFIGURE_CERT_H_

#include <common/configure_base.h>

namespace bumo {
	class CertDbConfigure {
	public:
		CertDbConfigure();
		~CertDbConfigure();

		std::string cert_db_path_;
		std::string rational_string_;
		std::string rational_db_type_;
		std::string tmp_path_;
		bool async_write_sql_;
		bool async_write_kv_;
		bool Load(const Json::Value &value);
	};

	class CertServerConfigure {
	public:
		CertServerConfigure();
		~CertServerConfigure();

		utils::InetAddressList listen_addresses_;

		std::string directory_;
		std::string index_name_;
		bool ssl_enable_;
		uint32_t query_limit_;
		uint32_t multiquery_limit_;
		SSLConfigure ssl_configure_;
		uint32_t thread_count_;
		std::string allow_origin_;
		bool Load(const Json::Value &value);
	};

	class CertConfigure : public ConfigureBase, public utils::Singleton<CertConfigure> {
		friend class utils::Singleton<CertConfigure>;
		CertConfigure();
		~CertConfigure();

	public:
		CertDbConfigure cert_db_configure_;
		LoggerConfigure logger_configure_;
		CertServerConfigure webserver_configure_;

		virtual bool LoadFromJson(const Json::Value &values);
	};
}

#endif
