#ifndef STORAGE_CERT_H_
#define STORAGE_CERT_H_

#include <common/storage.h>
#include "cert_configure.h"

namespace bumo {
	class CertStorage : public utils::Singleton<bumo::CertStorage> {
		friend class utils::Singleton<CertStorage>;
	private:
		CertStorage();
		~CertStorage();

		KeyValueDb *cert_db_;

		bool CloseDb();
		bool DescribeTable(const std::string &name, const std::string &sql_create_table);
		bool ManualDescribeTables();

		KeyValueDb *NewKeyValueDb(const CertDbConfigure &db_config);
	public:
		bool Initialize(const CertDbConfigure &db_config, bool bdropdb);
		bool Exit();

		KeyValueDb *cert_db();    //Store transactions and ledgers.

		//Lock the account db and ledger db to make the databases in synchronization.
		utils::ReadWriteLock cert_lock_;
	};
}

#endif
