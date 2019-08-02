#include <utils/headers.h>
#include "cert_server.h"

namespace bumo {

	CertServer::CertServer() :
		server_ptr_(NULL),
		context_(NULL),
		running(NULL),
		thread_count_(0),
		port_(0)
	{
	}

	CertServer::~CertServer() {
	}

	bool CertServer::Initialize(CertServerConfigure &webserver_config) {

		if (webserver_config.listen_addresses_.size() == 0) {
			LOG_INFO("No network address configured for listening, ignore it");
			return true;
		}

		if (webserver_config.ssl_enable_) {
			std::string strHome = utils::File::GetBinHome();
			context_ = new asio::ssl::context(asio::ssl::context::tlsv12);
			context_->set_options(
				asio::ssl::context::default_workarounds
				| asio::ssl::context::no_sslv2
				| asio::ssl::context::single_dh_use);
			context_->set_password_callback(std::bind(&CertServer::GetCertPassword, this, std::placeholders::_1, std::placeholders::_2));
			context_->use_certificate_chain_file(utils::String::Format("%s/%s", strHome.c_str(), webserver_config.ssl_configure_.chain_file_.c_str()));
			asio::error_code ignore_code;
			context_->use_private_key_file(utils::String::Format("%s/%s", strHome.c_str(), webserver_config.ssl_configure_.private_key_file_.c_str()),
				asio::ssl::context::pem,
				ignore_code);
			context_->use_tmp_dh_file(utils::String::Format("%s/%s", strHome.c_str(), webserver_config.ssl_configure_.dhparam_file_.c_str()));
		}

		thread_count_ = webserver_config.thread_count_;
		if (thread_count_ == 0) {
			thread_count_ = utils::System::GetCpuCoreCount() * 4;
		}

		utils::InetAddress address = webserver_config.listen_addresses_.front();
		try {
			server_ptr_ = new http::server::server(address.ToIp(), address.GetPort(), context_, thread_count_);
		}
		catch (std::exception& e) {
			LOG_ERROR("Failed to initialize web server, %s", e.what());
			return false;
		}

		port_ = server_ptr_->GetServerPort();

		server_ptr_->SetAllowOrigin(webserver_config.allow_origin_);
		server_ptr_->SetHome(utils::File::GetBinHome() + "/" + webserver_config.directory_);

		server_ptr_->add404(std::bind(&CertServer::FileNotFound, this, std::placeholders::_1, std::placeholders::_2));

		server_ptr_->addRoute("hello", std::bind(&CertServer::Hello, this, std::placeholders::_1, std::placeholders::_2));

		server_ptr_->addRoute("cancel_cert", std::bind(&CertServer::CancelCert, this, std::placeholders::_1, std::placeholders::_2));

		server_ptr_->addRoute("freeze_cert", std::bind(&CertServer::FreezeCert, this, std::placeholders::_1, std::placeholders::_2));

		server_ptr_->addRoute("thaw_cert", std::bind(&CertServer::ThawCert, this, std::placeholders::_1, std::placeholders::_2));

		server_ptr_->addRoute("get_cert_status", std::bind(&CertServer::GetCertStatus, this, std::placeholders::_1, std::placeholders::_2));


		server_ptr_->Run();
		running = true;

		LOG_INFO("Webserver started, thread count(" FMT_SIZE ") listen at %s", thread_count_, address.ToIpPort().c_str());
		return true;
	}

	bool CertServer::Exit() {
		LOG_INFO("WebServer stoping...");
		running = false;
		if (server_ptr_) {
			server_ptr_->Stop();
			delete server_ptr_;
			server_ptr_ = NULL;
		}

		if (context_) {
			delete context_;
			context_ = NULL;
		}
		LOG_INFO("WebServer stop [OK]");
		return true;
	}

	std::string CertServer::GetCertPassword(std::size_t, asio::ssl::context_base::password_purpose purpose) {
		return bumo::CertConfigure::Instance().webserver_configure_.ssl_configure_.private_password_;
	}

	void CertServer::FileNotFound(const http::server::request &request, std::string &reply) {
		reply = "File not found";
	}

	void CertServer::Hello(const http::server::request &request, std::string &reply) {
		Json::Value reply_json = Json::Value(Json::objectValue);
		reply_json["cert_manager_version"] = "1.0";
		reply_json["current_time"] = utils::Timestamp::Now().ToFormatString(true);
		reply = reply_json.toFastString();
	}

	void CertServer::CancelCert(const http::server::request &request, std::string &reply) {
		Json::Value body;
		Json::Value reply_json;
		if (!body.fromString(request.body)) {
			LOG_ERROR("Failed to parse the json content of the request");
			reply_json["error_desc"] = "request must be in json format";
			reply_json["error_code"] = Json::UInt(1);
			reply = reply_json.toStyledString();
			return;
		}

		std::string status = "";
		KeyValueDb *db = CertStorage::Instance().cert_db();
		int32_t ret = db->Get(body["serial"].asString(), status);
		if (ret > 0 && status.compare("1") == 0) {
			LOG_ERROR("The certificate (%s) was canceled", body["serial"].asCString());
			reply_json["error_desc"] = "the certificate was canceled";
			reply_json["error_code"] = Json::UInt(1);
			reply = reply_json.toStyledString();
			return;
		}

		std::string cancelStatus = "1";
		if (!db->Put(body["serial"].asString(), cancelStatus)) {
			LOG_ERROR("Failed to cancel the cert (%s), error desc(%s)", body["serial"].asCString(), db->error_desc().c_str());

		}
		reply_json["error_desc"] = "";
		reply_json["error_code"] = Json::UInt(0);
		reply = reply_json.toStyledString();
		return;
	}

	void CertServer::FreezeCert(const http::server::request &request, std::string &reply) {
		Json::Value body;
		Json::Value reply_json;
		if (!body.fromString(request.body)) {
			LOG_ERROR("Failed to parse the json content of the request");
			reply_json["error_desc"] = "request must be in json format";
			reply_json["error_code"] = Json::UInt(1);
			reply = reply_json.toStyledString();
			return;
		}

		std::string status = "";
		KeyValueDb *db = CertStorage::Instance().cert_db();
		int32_t ret = db->Get(body["serial"].asString(), status);
		if (ret > 0 && status.compare("1") == 0) {
			LOG_ERROR("The certificate (%s) was canceled", body["serial"].asCString());
			reply_json["error_desc"] = "the certificate was canceled";
			reply_json["error_code"] = Json::UInt(1);
			reply = reply_json.toStyledString();
			return;
		}
		if (ret > 0 && status.compare("2") == 1) {
			LOG_ERROR("The certificate (%s) was frozen", body["serial"].asCString());
			reply_json["error_desc"] = "the certificate was frozen";
			reply_json["error_code"] = Json::UInt(1);
			reply = reply_json.toStyledString();
			return;
		}

		std::string frozenStatus = "2";
		if (!db->Put(body["serial"].asString(), frozenStatus)) {
			LOG_ERROR("Failed to freeze the cert (%s), error desc(%s)", body["serial"].asCString(), db->error_desc().c_str());

		}
		reply_json["error_desc"] = "";
		reply_json["error_code"] = Json::UInt(0);
		reply = reply_json.toStyledString();
		return;
	}

	void CertServer::ThawCert(const http::server::request &request, std::string &reply) {
		Json::Value body;
		Json::Value reply_json;
		if (!body.fromString(request.body)) {
			LOG_ERROR("Failed to parse the json content of the request");
			reply_json["error_desc"] = "request must be in json format";
			reply_json["error_code"] = Json::UInt(1);
			reply = reply_json.toStyledString();
			return;
		}

		std::string status = "";
		KeyValueDb *db = CertStorage::Instance().cert_db();
		int32_t ret = db->Get(body["serial"].asString(), status);
		if (ret > 0 && status.compare("2") != 0 || ret <= 0) {
			LOG_ERROR("The certificate (%s) was not frozen", body["serial"].asCString());
			reply_json["error_desc"] = "the certificate was not frozen";
			reply_json["error_code"] = Json::UInt(1);
			reply = reply_json.toStyledString();
			return;
		}

		std::string thawStatus = "0";
		if (!db->Put(body["serial"].asString(), thawStatus)) {
			LOG_ERROR("Failed to thaw the cert (%s), error desc(%s)", body["serial"].asCString(), db->error_desc().c_str());

		}
		reply_json["error_desc"] = "";
		reply_json["error_code"] = Json::UInt(0);
		reply = reply_json.toStyledString();
		return;
	}

	void CertServer::GetCertStatus(const http::server::request &request, std::string &reply) {
		Json::Value reply_json;
		std::string cert_serial = request.GetParamValue("serial");
		KeyValueDb *db = CertStorage::Instance().cert_db();
		std::string status = "";
		//serial.resize(34);
		int32_t ret = db->Get(cert_serial, status);
		if (ret <= 0) {
			reply_json["error_desc"] = "Failed to get the serial of a cert";
			reply_json["error_code"] = Json::UInt(1);
			reply = reply_json.toFastString();
			return;
		}
		reply_json["error_desc"] = "";
		reply_json["error_code"] = Json::UInt(0);
		reply_json["result"]["status"] = status;
		reply = reply_json.toFastString();
		return;
	}
}