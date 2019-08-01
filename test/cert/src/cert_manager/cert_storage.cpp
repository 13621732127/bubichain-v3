#include <utils/strings.h>
#include <utils/logger.h>
#include <utils/file.h>
#include "cert_storage.h"
#define BUMO_ROCKSDB_MAX_OPEN_FILES 5000

namespace bumo {

	CertStorage::CertStorage() {
		cert_db_ = NULL;
	}

	CertStorage::~CertStorage() {}

	bool CertStorage::Initialize(const CertDbConfigure &db_config, bool bdropdb) {
	
		do {
			std::string strConnect = "";
			std::string str_dbname = "";
			std::vector<std::string> nparas = utils::String::split(db_config.rational_string_, " ");
			for (std::size_t i = 0; i < nparas.size(); i++) {
				std::string str = nparas[i];
				std::vector<std::string> n = utils::String::split(str, "=");
				if (n.size() == 2) {
					if (n[0] != "dbname") {
						strConnect += " ";
						strConnect += str;
					}
					else {
						str_dbname = n[1];
					}
				}
			}

			if (bdropdb) {
				bool do_success = false;
				do {
					//Check only for linux or mac whether the account db can be opened.
#ifndef WIN32
					KeyValueDb *account_db = NewKeyValueDb(db_config);
					if (!account_db->Open(db_config.cert_db_path_, -1)) {
						LOG_ERROR("Failed to drop db.Error description(%s)", account_db->error_desc().c_str());
						delete account_db;
						break;
					}
					account_db->Close();
					delete account_db;
#endif
					if (utils::File::IsExist(db_config.cert_db_path_) && !utils::File::DeleteFolder(db_config.cert_db_path_)) {
						LOG_ERROR_ERRNO("Failed to delete keyvalue db", STD_ERR_CODE, STD_ERR_DESC);
						break;
					}
					
					LOG_INFO("Drop db successful");
					do_success = true;
				} while (false);

				return do_success;
			}

			int32_t keyvaule_max_open_files = -1;
			int32_t ledger_max_open_files = -1;
			int32_t account_max_open_files = -1;
#ifdef OS_MAC
			FILE* fp = NULL;
			char ulimit_cmd[1024] = {0};
			char ulimit_result[1024] = {0};
			sprintf(ulimit_cmd, "ulimit -n");
			if ((fp = popen(ulimit_cmd, "r")) != NULL)
			{
				fgets(ulimit_result, sizeof(ulimit_result), fp);
				pclose(fp);
			}
			int32_t max_open_files = utils::String::Stoi(std::string(ulimit_result));
			if (max_open_files <= 100)
			{
				keyvaule_max_open_files = 2;
				ledger_max_open_files = 4;
				account_max_open_files = 4; 
				LOG_INFO("In mac os, the maximum number (%d) of files handles is too low.", max_open_files);
			}
			else
			{
				keyvaule_max_open_files = 2 + (max_open_files - 100) * 0.2;
				ledger_max_open_files = 4 + (max_open_files - 100) * 0.4;
				account_max_open_files = 4 + (max_open_files - 100) * 0.4;

				keyvaule_max_open_files = (keyvaule_max_open_files > BUMO_ROCKSDB_MAX_OPEN_FILES) ? -1 : keyvaule_max_open_files;
				ledger_max_open_files = (ledger_max_open_files > BUMO_ROCKSDB_MAX_OPEN_FILES) ? -1 : ledger_max_open_files;
				account_max_open_files = (account_max_open_files > BUMO_ROCKSDB_MAX_OPEN_FILES) ? -1 : account_max_open_files;
			}
			LOG_INFO("Assigned number of file handles in mac os, max :%d, keyvaule used:%d, ledger used:%d, account used:%d:",
				max_open_files, keyvaule_max_open_files, ledger_max_open_files, account_max_open_files);
#endif
			cert_db_ = NewKeyValueDb(db_config);
			if (!cert_db_->Open(db_config.cert_db_path_, keyvaule_max_open_files)) {
				LOG_ERROR("Failed to open keyvalue db path(%s), the reason is(%s)\n",
					db_config.cert_db_path_.c_str(), cert_db_->error_desc().c_str());
				break;
			}
			return true;

		} while (false);

		CloseDb();
		return false;
	}


	bool  CertStorage::CloseDb() {
		bool ret1 = true, ret2 = true, ret3 = true;
		if (cert_db_ != NULL) {
			ret1 = cert_db_->Close();
			delete cert_db_;
			cert_db_ = NULL;
		}

		return ret1 && ret2 && ret3;
	}

	bool CertStorage::Exit() {
		return CloseDb();
	}

	KeyValueDb *CertStorage::cert_db() {
		return cert_db_;
	}

	KeyValueDb *CertStorage::NewKeyValueDb(const CertDbConfigure &db_config) {
		KeyValueDb *db = NULL;
#ifdef WIN32
		db = new LevelDbDriver();
#else
		db = new RocksDbDriver();
#endif

		return db;
	}
}
