#include <utils/headers.h>
#include "cert_storage.h"
#include <common/argument.h>
#include "cert_server.h"
#include "cert_configure.h"

void SaveWSPort();
void RunLoop();
int main(int argc, char *argv[]){

#ifdef WIN32
	_set_output_format(_TWO_DIGIT_EXPONENT);
#else
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	size_t stacksize = 0;
	int ret = pthread_attr_getstacksize(&attr, &stacksize);
	if (ret != 0) {
		printf("get stacksize error!:%d\n", (int)stacksize);
		return -1;
	}

	if (stacksize <= 2 * 1024 * 1024)
	{
		stacksize = 2 * 1024 * 1024;

		pthread_attr_t object_attr;
		pthread_attr_init(&object_attr);
		ret = pthread_attr_setstacksize(&object_attr, stacksize);
		if (ret != 0) {
			printf("set main stacksize error!:%d\n", (int)stacksize);
			return -1;
		}
	}
#endif

	utils::SetExceptionHandle();
	utils::Thread::SetCurrentThreadName("bumo-thread");

	utils::net::Initialize();
	utils::Timer::InitInstance();
	bumo::CertConfigure::InitInstance();
	bumo::CertStorage::InitInstance();
	utils::Logger::InitInstance();
	bumo::CertServer::InitInstance();

	do {
		utils::ObjectExit object_exit;
		bumo::InstallSignal();

		srand((uint32_t)time(NULL));

		bumo::CertConfigure &config = bumo::CertConfigure::Instance();
		std::string config_path = "config/cert.json";
		if (!utils::File::IsAbsolute(config_path)){
			config_path = utils::String::Format("%s/%s", utils::File::GetBinHome().c_str(), config_path.c_str());
		}

		if (!config.Load(config_path)){
			LOG_STD_ERRNO("Failed to load configuration", STD_ERR_CODE, STD_ERR_DESC);
			break;
		}

		std::string log_path = config.logger_configure_.path_;
		if (!utils::File::IsAbsolute(log_path)){
			log_path = utils::String::Format("%s/%s", utils::File::GetBinHome().c_str(), log_path.c_str());
		}
		const bumo::LoggerConfigure &logger_config = bumo::CertConfigure::Instance().logger_configure_;
		utils::Logger &logger = utils::Logger::Instance();
		logger.SetCapacity(logger_config.time_capacity_, logger_config.size_capacity_);
		logger.SetExpireDays(logger_config.expire_days_);
		if (!bumo::g_enable_ || !logger.Initialize((utils::LogDest)(logger_config.dest_),
			(utils::LogLevel)logger_config.level_, log_path, true)){
			LOG_STD_ERR("Failed to initialize logger");
			break;
		}
		object_exit.Push(std::bind(&utils::Logger::Exit, &logger));
		LOG_INFO("Initialized daemon successfully");
		LOG_INFO("Loaded configure successfully");
		LOG_INFO("Initialized logger successfully");

		// end run command
		bumo::CertStorage &storage = bumo::CertStorage::Instance();
		LOG_INFO("The path of the database is as follows: cert_db(%s)", config.cert_db_configure_.cert_db_path_.c_str());

		if (!bumo::g_enable_ || !storage.Initialize(config.cert_db_configure_, false)) {
			LOG_ERROR("Failed to initialize database");
			break;
		}
		object_exit.Push(std::bind(&bumo::CertStorage::Exit, &storage));
		LOG_INFO("Initialized database successfully");

		bumo::CertServer &web_server = bumo::CertServer::Instance();
		if (!bumo::g_enable_ || !web_server.Initialize(bumo::CertConfigure::Instance().webserver_configure_)) {
			LOG_ERROR("Failed to initialize web server");
			break;
		}
		object_exit.Push(std::bind(&bumo::CertServer::Exit, &web_server));
		LOG_INFO("Initialized web server successfully");

		SaveWSPort();

		bumo::g_ready_ = true;

		RunLoop();

		LOG_INFO("Process begins to quit...");

	} while (false);

	bumo::CertServer::ExitInstance();
	bumo::CertConfigure::ExitInstance();
	bumo::CertStorage::ExitInstance();
	utils::Logger::ExitInstance();

	printf("process exit\n");
}

void RunLoop(){
	int64_t check_module_interval = 5 * utils::MICRO_UNITS_PER_SEC;
	int64_t last_check_module = 0;
	while (bumo::g_enable_){
		utils::Logger::Instance().CheckExpiredLog();
		utils::Sleep(1);
	}
}

void SaveWSPort(){
	std::string tmp_file = utils::File::GetTempDirectory() + "/bumo_listen_port";
	Json::Value json_port = Json::Value(Json::objectValue);
	//json_port["webserver_port"] = bumo::WebServer::Instance().GetListenPort();
	utils::File file;
	if (file.Open(tmp_file, utils::File::FILE_M_WRITE | utils::File::FILE_M_TEXT))
	{
		std::string line = json_port.toFastString();
		file.Write(line.c_str(), 1, line.length());
		file.Close();
	}
}
