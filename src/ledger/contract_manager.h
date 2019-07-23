#ifndef CONTRACT_MANAGER_H_
#define CONTRACT_MANAGER_H_
#include <map>
#include <string>

#include <utils/headers.h>
#include <v8.h>
#include <libplatform/libplatform.h>
#include <libplatform/libplatform-export.h>
#include <proto/cpp/chain.pb.h>

namespace bubi{

	class ContractManager 
	{
		v8::Isolate* isolate_;
		v8::Global<v8::Context> g_context_;
		v8::Local<v8::ObjectTemplate> global_;

	    static std::map<std::string, std::string> jslib_sources;
		static const std::string sender_name_ ;
		static const std::string this_address_;
		static const char* main_name_;
		static const std::string trigger_tx_name_;
		static const std::string trigger_tx_index_name_;
		static const std::string this_header_name_;

		static const std::string tx_hash_;

		static v8::Platform* 	platform_;
		static v8::Isolate::CreateParams create_params_;
		
	public:
		static ContractManager* executing_contract_;
		int tx_do_count_;
	public:
		
		ContractManager();
		~ContractManager();

		static void Initialize(int argc, char** argv);

		bool Execute(const std::string& code, 
			const std::string &input, 
			const std::string& thisAddress, 
			const std::string& sender,
			const std::string& trigger_tx,
			const std::string& tx_hash,
			int32_t index,
			const std::string& consensus_value,
			std::string& error_msg);

		bool SourceCodeCheck(const std::string& code, std::string& err_msg);

		bool Exit();
	private:
		static bool LoadJsLibSource();
		static v8::Local<v8::Context> CreateContext(v8::Isolate* isolate);
		static std::string ReportException(v8::Isolate* isolate, v8::TryCatch* try_catch);

		static const char* ToCString(const v8::String::Utf8Value& value);

		static void CallBackLog(const v8::FunctionCallbackInfo<v8::Value>& args);

		static void CallBackGetAccountMetaData(const v8::FunctionCallbackInfo<v8::Value>& args);

		static void CallBackSetAccountMetaData(const v8::FunctionCallbackInfo<v8::Value>& args);

		static void CallBackGetAccountAsset(const v8::FunctionCallbackInfo<v8::Value>& args);

		static void Include(const v8::FunctionCallbackInfo<v8::Value>& args);

		//get account info from an account
		static void CallBackGetAccountInfo(const v8::FunctionCallbackInfo<v8::Value>& args);

		//get a ledger info from a ledger
		static void CallBackGetLedgerInfo(const v8::FunctionCallbackInfo<v8::Value>& args);

		//get transaction info from a transaction
		static void CallBackGetTransactionInfo(const v8::FunctionCallbackInfo<v8::Value>& args);

		//static void CallBackGetThisAddress(const v8::FunctionCallbackInfo<v8::Value>& args);

		//make a transaction
		static void CallBackDoOperation(const v8::FunctionCallbackInfo<v8::Value>& args);

		static ContractManager* UnwrapContract(v8::Local<v8::Object> obj);

		//static bool DoTransaction(protocol::TransactionEnv& env);

		//str to int64 check
		static void CallBackStoI64Check(const v8::FunctionCallbackInfo<v8::Value>& args);
		//Int64 add
		static void CallBackInt64Add(const v8::FunctionCallbackInfo<v8::Value>& args);
		//Int64 sub
		static void CallBackInt64Sub(const v8::FunctionCallbackInfo<v8::Value>& args);
		//Int64 mul
		static void CallBackInt64Mul(const v8::FunctionCallbackInfo<v8::Value>& args);
		//Int64 mod
		static void CallBackInt64Mod(const v8::FunctionCallbackInfo<v8::Value>& args);
		//Int64 div
		static void CallBackInt64Div(const v8::FunctionCallbackInfo<v8::Value>& args);
		//Int64 compare
		static void CallBackInt64Compare(const v8::FunctionCallbackInfo<v8::Value>& args);
		//Assert an expression.
		static void CallBackAssert(const v8::FunctionCallbackInfo<v8::Value>& args);
		//Check address valid
		static void CallBackAddressValidCheck(const v8::FunctionCallbackInfo<v8::Value>& args);

		//Get the hash of one of the 1024 most recent complete blocks
		static void CallBackSha256(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void CallBackVerify(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void CallBackToAddress(const v8::FunctionCallbackInfo<v8::Value>& args);

		typedef enum tagDataEncodeType {
			BASE16 = 0,
			RAW_DATA = 1,
			BASE64 = 2
		}DataEncodeType;

		static bool TransEncodeType(const v8::Local<v8::Value> &arg, DataEncodeType &data_type);
		static bool TransEncodeData(const v8::Local<v8::Value> &raw_data, const DataEncodeType &encode_type, std::string &result_data);
	};
}
#endif