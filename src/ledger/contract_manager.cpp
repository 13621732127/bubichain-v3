#include <utils/logger.h>
#include <utils/base64.h>
#include <common/pb2json.h>
#include <api/websocket_server.h>
#include "contract_manager.h"
#include "ledger_frm.h"
#include "ledger_manager.h"


namespace bubi{

	std::map<std::string, std::string> ContractManager::jslib_sources;
	const std::string ContractManager::sender_name_ = "sender";
	const std::string ContractManager::this_address_ = "thisAddress";
	const char* ContractManager::main_name_ = "main";
	const std::string ContractManager::trigger_tx_name_ = "trigger";
	const std::string ContractManager::trigger_tx_index_name_ = "triggerIndex";
	const std::string ContractManager::this_header_name_ = "consensusValue";
	const std::string ContractManager::tx_hash_ = "tx_hash";

	v8::Platform* ContractManager::platform_ = nullptr;
	v8::Isolate::CreateParams ContractManager::create_params_;
	ContractManager* ContractManager::executing_contract_ = nullptr;


	ContractManager::ContractManager(){
		tx_do_count_ = 0;
		isolate_ = v8::Isolate::New(create_params_);
	}

	ContractManager::~ContractManager(){
		isolate_->Dispose();
		isolate_ = NULL;
	}

	void ContractManager::Initialize(int argc, char** argv){
		LoadJsLibSource();
		platform_ = v8::platform::CreateDefaultPlatform();
		v8::V8::InitializeExternalStartupData(argv[0]);
		v8::V8::InitializePlatform(platform_);
		if (!v8::V8::Initialize()){
			BUBI_EXIT("v8 Initialize fail");
		}
		create_params_.array_buffer_allocator =
			v8::ArrayBuffer::Allocator::NewDefaultAllocator();
	}

	v8::Local<v8::Context> ContractManager::CreateContext(v8::Isolate* isolate) {
		// Create a template for the global object.
		v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);
		// Bind the global 'print' function to the C++ Print callback.
		global->Set(
			v8::String::NewFromUtf8(isolate, "callBackLog", v8::NewStringType::kNormal)
			.ToLocalChecked(),
			v8::FunctionTemplate::New(isolate, ContractManager::CallBackLog));

		global->Set(
			v8::String::NewFromUtf8(isolate, "callBackGetAccountInfo", v8::NewStringType::kNormal)
			.ToLocalChecked(),
			v8::FunctionTemplate::New(isolate, ContractManager::CallBackGetAccountInfo));


		global->Set(
			v8::String::NewFromUtf8(isolate, "callBackGetAccountAsset", v8::NewStringType::kNormal)
			.ToLocalChecked(),
			v8::FunctionTemplate::New(isolate, ContractManager::CallBackGetAccountAsset));

		global->Set(
			v8::String::NewFromUtf8(isolate, "callBackGetAccountMetaData", v8::NewStringType::kNormal)
			.ToLocalChecked(),
			v8::FunctionTemplate::New(isolate, ContractManager::CallBackGetAccountMetaData));


		global->Set(
			v8::String::NewFromUtf8(isolate, "callBackSetAccountMetaData", v8::NewStringType::kNormal)
			.ToLocalChecked(),
			v8::FunctionTemplate::New(isolate, ContractManager::CallBackSetAccountMetaData));

		global->Set(
			v8::String::NewFromUtf8(isolate, "callBackGetLedgerInfo", v8::NewStringType::kNormal)
			.ToLocalChecked(),
			v8::FunctionTemplate::New(isolate, ContractManager::CallBackGetLedgerInfo));

		/*		global->Set(
		v8::String::NewFromUtf8(isolate_, "callBackGetTransactionInfo", v8::NewStringType::kNormal)
		.ToLocalChecked(),
		v8::FunctionTemplate::New(isolate_, ContractManager::CallBackGetTransactionInfo, v8::External::New(isolate_, this)));*/

		global->Set(
			v8::String::NewFromUtf8(isolate, "callBackDoOperation", v8::NewStringType::kNormal)
			.ToLocalChecked(),
			v8::FunctionTemplate::New(isolate, ContractManager::CallBackDoOperation));

		global->Set(
			v8::String::NewFromUtf8(isolate, "include", v8::NewStringType::kNormal)
			.ToLocalChecked(),
			v8::FunctionTemplate::New(isolate, ContractManager::Include));

		//add 3001 version
		if (CHECK_VERSION_GT_3001) {
			global->Set(
				v8::String::NewFromUtf8(isolate, "stoI64Check", v8::NewStringType::kNormal)
				.ToLocalChecked(),
				v8::FunctionTemplate::New(isolate, ContractManager::CallBackStoI64Check));
			global->Set(
				v8::String::NewFromUtf8(isolate, "int64Add", v8::NewStringType::kNormal)
				.ToLocalChecked(),
				v8::FunctionTemplate::New(isolate, ContractManager::CallBackInt64Add));
			global->Set(
				v8::String::NewFromUtf8(isolate, "int64Sub", v8::NewStringType::kNormal)
				.ToLocalChecked(),
				v8::FunctionTemplate::New(isolate, ContractManager::CallBackInt64Sub));
			global->Set(
				v8::String::NewFromUtf8(isolate, "int64Mul", v8::NewStringType::kNormal)
				.ToLocalChecked(),
				v8::FunctionTemplate::New(isolate, ContractManager::CallBackInt64Mul));
			global->Set(
				v8::String::NewFromUtf8(isolate, "int64Mod", v8::NewStringType::kNormal)
				.ToLocalChecked(),
				v8::FunctionTemplate::New(isolate, ContractManager::CallBackInt64Mod));
			global->Set(
				v8::String::NewFromUtf8(isolate, "int64Div", v8::NewStringType::kNormal)
				.ToLocalChecked(),
				v8::FunctionTemplate::New(isolate, ContractManager::CallBackInt64Div));
			global->Set(
				v8::String::NewFromUtf8(isolate, "int64Compare", v8::NewStringType::kNormal)
				.ToLocalChecked(),
				v8::FunctionTemplate::New(isolate, ContractManager::CallBackInt64Compare));
			global->Set(
				v8::String::NewFromUtf8(isolate, "assert", v8::NewStringType::kNormal)
				.ToLocalChecked(),
				v8::FunctionTemplate::New(isolate, ContractManager::CallBackAssert));
			global->Set(
				v8::String::NewFromUtf8(isolate, "addressCheck", v8::NewStringType::kNormal)
				.ToLocalChecked(),
				v8::FunctionTemplate::New(isolate, ContractManager::CallBackAddressValidCheck));
			global->Set(
				v8::String::NewFromUtf8(isolate, "sha256", v8::NewStringType::kNormal)
				.ToLocalChecked(),
				v8::FunctionTemplate::New(isolate, ContractManager::CallBackSha256));
			global->Set(
				v8::String::NewFromUtf8(isolate, "ecVerify", v8::NewStringType::kNormal)
				.ToLocalChecked(),
				v8::FunctionTemplate::New(isolate, ContractManager::CallBackVerify));
			global->Set(
				v8::String::NewFromUtf8(isolate, "toAddress", v8::NewStringType::kNormal)
				.ToLocalChecked(),
				v8::FunctionTemplate::New(isolate, ContractManager::CallBackToAddress));
		}

		return v8::Context::New(isolate, NULL, global);
	}

	std::string ContractManager::ReportException(v8::Isolate* isolate, v8::TryCatch* try_catch) {
		v8::HandleScope handle_scope(isolate);
		v8::String::Utf8Value exception(try_catch->Exception());
		const char* exception_string = ToCString(exception);
		std::string exec_string(exception_string);
		exec_string.resize(256);
		Json::Value json_result;

		v8::Local<v8::Message> message = try_catch->Message();
		std::string error_msg;
		if (message.IsEmpty()) {
			// V8 didn't provide any extra information about this error; just
			// print the exception.
			json_result["exception"] = error_msg;
		}
		else {
			// Print (filename):(line number): (message).
			v8::String::Utf8Value filename(message->GetScriptOrigin().ResourceName());
			v8::Local<v8::Context> context(isolate->GetCurrentContext());
			const char* filename_string = ToCString(filename);
			int linenum = message->GetLineNumber(context).FromJust();
			json_result["filename"] = filename_string;
			json_result["linenum"] = linenum;
			json_result["exception"] = exec_string;

			//print error stack
			v8::Local<v8::Value> stack_trace_string;
			if (try_catch->StackTrace(context).ToLocal(&stack_trace_string) &&
				stack_trace_string->IsString() &&
				v8::Local<v8::String>::Cast(stack_trace_string)->Length() > 0) {
				v8::String::Utf8Value stack_trace(stack_trace_string);
				const char* stack_trace_string = ToCString(stack_trace);
				json_result["stack"] = stack_trace_string;
			}
		}
		return json_result.toFastString();
	}

	bool ContractManager::SourceCodeCheck(const std::string& code, std::string& err_msg){

		v8::Isolate::Scope isolate_scope(isolate_);

		v8::HandleScope handle_scope(isolate_);
		v8::TryCatch try_catch(isolate_);

		v8::Local<v8::ObjectTemplate> templ =
			v8::Local<v8::ObjectTemplate>::New(isolate_, global_);

		v8::Handle<v8::Context> context = v8::Context::New(isolate_, NULL, templ);
		v8::Context::Scope context_scope(context);

		
		auto string_sender = v8::String::NewFromUtf8(isolate_, "", v8::NewStringType::kNormal).ToLocalChecked();
		context->Global()->Set(context, v8::String::NewFromUtf8(isolate_, sender_name_.c_str(), v8::NewStringType::kNormal).ToLocalChecked(), string_sender);


		auto string_contractor = v8::String::NewFromUtf8(isolate_, "", v8::NewStringType::kNormal).ToLocalChecked();
		context->Global()->Set(context, v8::String::NewFromUtf8(isolate_, this_address_.c_str(), v8::NewStringType::kNormal).ToLocalChecked(), string_contractor);

		if (CHECK_VERSION_GT_3001) {
			auto string_tx_hash = v8::String::NewFromUtf8(isolate_, "", v8::NewStringType::kNormal).ToLocalChecked();
			context->Global()->Set(context, v8::String::NewFromUtf8(isolate_, tx_hash_.c_str(), v8::NewStringType::kNormal).ToLocalChecked(), string_tx_hash);
		}

		auto str_json_v8 = v8::String::NewFromUtf8(isolate_, "{}", v8::NewStringType::kNormal).ToLocalChecked();
		auto tx_v8 = v8::JSON::Parse(str_json_v8);
		context->Global()->Set(context,
			v8::String::NewFromUtf8(isolate_, trigger_tx_name_.c_str(), v8::NewStringType::kNormal).ToLocalChecked(),
			tx_v8);

		v8::Local<v8::Integer> index_v8 = v8::Int32::New(isolate_, 0);
		context->Global()->Set(context,
			v8::String::NewFromUtf8(isolate_, trigger_tx_index_name_.c_str(), v8::NewStringType::kNormal).ToLocalChecked(),
			index_v8);

		auto v8_consensus_value = v8::String::NewFromUtf8(isolate_, "{}", v8::NewStringType::kNormal).ToLocalChecked();
		auto v8HeadJson = v8::JSON::Parse(v8_consensus_value);
		context->Global()->Set(context,
			v8::String::NewFromUtf8(isolate_, this_header_name_.c_str(), v8::NewStringType::kNormal).ToLocalChecked(),
			v8HeadJson);

		v8::Local<v8::String> v8src = v8::String::NewFromUtf8(isolate_, code.c_str());
		v8::Local<v8::Script> compiled_script;

		if (!v8::Script::Compile(context, v8src).ToLocal(&compiled_script)){
			err_msg = ReportException(isolate_, &try_catch);
			LOG_ERROR("%s", err_msg.c_str());
			return false;
		}

		/*
		auto result = compiled_script->Run(context).ToLocalChecked();

		v8::Local<v8::String> process_name =
		v8::String::NewFromUtf8(isolate_, main_name_
		, v8::NewStringType::kNormal, strlen(main_name_))
		.ToLocalChecked();


		v8::Local<v8::Value> process_val;

		if (!context->Global()->Get(context, process_name).ToLocal(&process_val) ) {
		err_msg = utils::String::Format("lost of %s function", main_name_);
		LOG_ERROR("%s", err_msg.c_str());
		return false;
		}

		if (!process_val->IsFunction()){
		err_msg = utils::String::Format("lost of %s function", main_name_);
		LOG_ERROR("%s", err_msg.c_str());
		return false;
		}
		*/
		return true;
	}

	bool ContractManager::Execute(const std::string& code, 
		const std::string &input,
		const std::string& token,
		const std::string& sender, 
		const std::string& trigger_tx, 
		const std::string& tx_hash,
		int32_t index,
		const std::string& consensus_value,
		std::string& error_msg)
	{
		v8::Isolate::Scope isolate_scope(isolate_);
		v8::HandleScope handle_scope(isolate_);
		v8::TryCatch try_catch(isolate_);

		v8::Local<v8::Context> context = CreateContext(isolate_);

		v8::Context::Scope context_scope(context);


		v8::Local<v8::Value> vtoken = v8::String::NewFromUtf8(isolate_, token.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
		context->SetSecurityToken(vtoken);

		auto string_sender = v8::String::NewFromUtf8(isolate_, sender.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
		context->Global()->Set(context,
			v8::String::NewFromUtf8(isolate_, sender_name_.c_str(), v8::NewStringType::kNormal).ToLocalChecked(),
			string_sender);

		auto string_contractor = v8::String::NewFromUtf8(isolate_, token.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
		context->Global()->Set(context,
			v8::String::NewFromUtf8(isolate_, this_address_.c_str(), v8::NewStringType::kNormal).ToLocalChecked(),
			string_contractor);

		if (CHECK_VERSION_GT_3001) {
			auto string_tx_hash = v8::String::NewFromUtf8(isolate_, tx_hash.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
			context->Global()->Set(context,
				v8::String::NewFromUtf8(isolate_, tx_hash_.c_str(), v8::NewStringType::kNormal).ToLocalChecked(),
				string_tx_hash);
		}


		auto str_json_v8 = v8::String::NewFromUtf8(isolate_, trigger_tx.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
		auto tx_v8 = v8::JSON::Parse(str_json_v8);
		context->Global()->Set(context,
			v8::String::NewFromUtf8(isolate_, trigger_tx_name_.c_str(), v8::NewStringType::kNormal).ToLocalChecked(),
			tx_v8);

		v8::Local<v8::Integer> index_v8 = v8::Int32::New(isolate_, index);
		context->Global()->Set(context,
			v8::String::NewFromUtf8(isolate_, trigger_tx_index_name_.c_str(), v8::NewStringType::kNormal).ToLocalChecked(),
			index_v8);

		auto v8_consensus_value = v8::String::NewFromUtf8(isolate_, consensus_value.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
		auto v8HeadJson = v8::JSON::Parse(v8_consensus_value);
		context->Global()->Set(context,
			v8::String::NewFromUtf8(isolate_, this_header_name_.c_str(), v8::NewStringType::kNormal).ToLocalChecked(),
			v8HeadJson);

		v8::Local<v8::String> v8src = v8::String::NewFromUtf8(isolate_, code.c_str());
		v8::Local<v8::Script> compiled_script;

		auto back = executing_contract_;
		do
		{
			executing_contract_ = this;
			if (!v8::Script::Compile(context, v8src).ToLocal(&compiled_script)){
				error_msg = ReportException(isolate_, &try_catch);
				break;
			}

			v8::Local<v8::Value> result;
			if (!compiled_script->Run(context).ToLocal(&result)){
				error_msg = ReportException(isolate_, &try_catch);
				break;
			}

			v8::Local<v8::String> process_name =
				v8::String::NewFromUtf8(isolate_, main_name_, v8::NewStringType::kNormal, strlen(main_name_))
				.ToLocalChecked();
			v8::Local<v8::Value> process_val;

			if (!context->Global()->Get(context, process_name).ToLocal(&process_val) ||
				!process_val->IsFunction()) {
				LOG_ERROR("lost of %s function", main_name_);
				break;
			}

			v8::Local<v8::Function> process = v8::Local<v8::Function>::Cast(process_val);

			const int argc = 1;
			v8::Local<v8::String> arg1 = v8::String::NewFromUtf8(isolate_, input.c_str(), v8::NewStringType::kNormal).ToLocalChecked();

			v8::Local<v8::Value> argv[argc];
			argv[0] = arg1;

			v8::Local<v8::Value> callresult;
			if (!process->Call(context, context->Global(), argc, argv).ToLocal(&callresult)){
				error_msg = ReportException(isolate_, &try_catch);
				break;
			}

			executing_contract_ = back;
			return true;
		} while (false);
		executing_contract_ = back;
		return false;
	}



	bool ContractManager::Exit(){
		return true;
	}


	bool ContractManager::LoadJsLibSource() {
		std::string lib_path = utils::String::Format("%s/jslib", utils::File::GetBinHome().c_str());
		utils::FileAttributes files;
		utils::File::GetFileList(lib_path, "*.js", files);
		for (utils::FileAttributes::iterator iter = files.begin(); iter != files.end(); iter++) {
			utils::FileAttribute attr = iter->second;
			utils::File file;
			std::string file_path = utils::String::Format("%s/%s", lib_path.c_str(), iter->first.c_str());
			if (!file.Open(file_path, utils::File::FILE_M_READ)) {
				LOG_ERROR_ERRNO("Open js lib file failed, path(%s)", file_path.c_str(), STD_ERR_CODE, STD_ERR_DESC);
				continue;
			}

			std::string data;
			if (file.ReadData(data, 10 * utils::BYTES_PER_MEGA) < 0) {
				LOG_ERROR_ERRNO("Read js lib file failed, path(%s)", file_path.c_str(), STD_ERR_CODE, STD_ERR_DESC);
				continue;
			}

			jslib_sources[iter->first] = data;
		}

		return true;
	}

	const char* ContractManager::ToCString(const v8::String::Utf8Value& value) {
		return *value ? *value : "<string conversion failed>";
	}

	void ContractManager::Include(const v8::FunctionCallbackInfo<v8::Value>& args) {
		do {
			if (args.Length() != 1) {
				LOG_ERROR("Parameter error, args length(%d) not equal to 1", args.Length());
				break;
			}

			if (!args[0]->IsString()) {
				LOG_ERROR("Include parameter error, parameter should be a String");
				break;
			}
			v8::String::Utf8Value str(args[0]);

			std::map<std::string, std::string>::iterator find_source = jslib_sources.find(*str);
			if (find_source == jslib_sources.end()) {
				LOG_ERROR("Can't find the include file(%s) in jslib directory", *str);
				break;
			}


			v8::TryCatch try_catch(args.GetIsolate());
			std::string js_file = find_source->second; //load_file(*str);

			v8::Local<v8::String> source = v8::String::NewFromUtf8(args.GetIsolate(), js_file.c_str());
			v8::Local<v8::Script> script;
			if (!v8::Script::Compile(args.GetIsolate()->GetCurrentContext(), source).ToLocal(&script)) {
				ReportException(args.GetIsolate(), &try_catch);
				break;
			}

			v8::Local<v8::Value> result;
			if (!script->Run(args.GetIsolate()->GetCurrentContext()).ToLocal(&result)) {
				ReportException(args.GetIsolate(), &try_catch);
				break;
			}

			args.GetReturnValue().Set(true);
			return;
		} while (false);
		args.GetReturnValue().Set(false);
		//return v8::Undefined(args.GetIsolate());
	}


	void ContractManager::CallBackGetAccountAsset(const v8::FunctionCallbackInfo<v8::Value>& args) {
		if (args.Length() != 2) {
			LOG_ERROR("Parameter error, args length(%d) not equal to 2", args.Length());
			args.GetReturnValue().Set(false);
			return;
		}

		do{
			v8::HandleScope handle_scope(args.GetIsolate());
			if (!args[0]->IsString()) {
				LOG_ERROR("contract execute error,CallBackGetAccountAsset, parameter 1 should be a String");
				break;
			}
			auto address = std::string(ToCString(v8::String::Utf8Value(args[0])));

			if (!args[1]->IsObject()) {
				LOG_ERROR("contract execute error,CallBackGetAccountAsset parameter 2 should be a object");
				break;
			}
			auto ss = v8::JSON::Stringify(args.GetIsolate()->GetCurrentContext(), args[1]->ToObject()).ToLocalChecked();
			auto strjson = std::string(ToCString(v8::String::Utf8Value(ss)));
			Json::Value json;
			json.fromString(strjson);

			protocol::AssetProperty property;
			std::string error_msg;
			if (!Json2Proto(json, property, error_msg)){
				LOG_ERROR("contract execute error,CallBackGetAccountAsset,parameter property not valid. error=%s", error_msg.c_str());
				break;
			}


			bubi::AccountFrm::pointer account_frm = nullptr;
			auto environment = LedgerManager::Instance().transaction_stack_.top()->environment_;
			if (!environment->GetEntry(address, account_frm)){
				break;
			}

			protocol::Asset asset;
			if (!account_frm->GetAsset(property, asset)){
				break;
			}

			Json::Value json_asset = bubi::Proto2Json(asset);
			std::string strvalue = json_asset.toFastString();

			v8::Local<v8::String> returnvalue = v8::String::NewFromUtf8(
				args.GetIsolate(), strvalue.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
			args.GetReturnValue().Set(v8::JSON::Parse(returnvalue));
			return;
		} while (false);
		args.GetReturnValue().Set(false);
	}



	void ContractManager::CallBackGetAccountMetaData(const v8::FunctionCallbackInfo<v8::Value>& args) {
		do
		{
			if (args.Length() != 2) {
				LOG_ERROR("Parameter error, args length(%d) not equal to 2", args.Length());
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString()) {
				LOG_ERROR("contract execute error,CallBackGetAccountStorage, parameter 0 should be a String");
				break;
			}

			v8::String::Utf8Value str(args[0]);
			std::string address(ToCString(str));

			if (!args[1]->IsString()) {
				LOG_ERROR("contract execute error,CallBackGetAccountStorage, parameter 1 should be a String");
				break;
			}
			std::string key = ToCString(v8::String::Utf8Value(args[1]));

			bubi::AccountFrm::pointer account_frm = nullptr;
			auto environment = LedgerManager::Instance().transaction_stack_.top()->environment_;
			if (!environment->GetEntry(address, account_frm)){
				break;
			}

			protocol::KeyPair kp;
			if (!account_frm->GetMetaData(key, kp)){
				break;
			}

			Json::Value json = bubi::Proto2Json(kp);
			std::string strvalue = json.toFastString();

			v8::Local<v8::String> returnvalue = v8::String::NewFromUtf8(
				args.GetIsolate(), strvalue.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
			args.GetReturnValue().Set(v8::JSON::Parse(returnvalue));
			return;
		} while (false);
		args.GetReturnValue().Set(false);
	}


	void ContractManager::CallBackSetAccountMetaData(const v8::FunctionCallbackInfo<v8::Value>& args){
		do
		{
			if (args.Length() != 1) {
				LOG_ERROR("Parameter error, args length(%d) not equal to 1", args.Length());
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());

			v8::String::Utf8Value token(args.GetIsolate()->GetCurrentContext()->GetSecurityToken()->ToString());
			std::string contractor(ToCString(token));

			if (!args[0]->IsObject()) {
				LOG_ERROR("contract execute error,CallBackSetAccountStorage, parameter 0 should be a object");
				break;
			}
			v8::Local<v8::String> str = v8::JSON::Stringify(args.GetIsolate()->GetCurrentContext(), args[0]->ToObject()).ToLocalChecked();
			v8::String::Utf8Value  utf8(str);

			protocol::TransactionEnv txenv;
			txenv.mutable_transaction()->set_source_address(contractor);
			protocol::Operation *ope = txenv.mutable_transaction()->add_operations();

			Json::Value json;
			if (!json.fromCString(ToCString(utf8))){
				LOG_ERROR("fromCString fail, fatal error");
				break;
			}

			ope->set_type(protocol::Operation_Type_SET_METADATA);
			protocol::OperationSetMetadata proto_setmetadata;
			std::string error_msg;
			if (!Json2Proto(json, proto_setmetadata, error_msg)){
				LOG_ERROR("json error=%s", error_msg.c_str());
				break;
			}
			ope->mutable_set_metadata()->CopyFrom(proto_setmetadata);
			LedgerManager::Instance().DoTransaction(txenv);
			args.GetReturnValue().Set(true);
			return;
		} while (false);
		args.GetReturnValue().Set(false);
	}


	//
	void ContractManager::CallBackGetAccountInfo(const v8::FunctionCallbackInfo<v8::Value>& args) {
		do
		{
			if (args.Length() != 1) {
				LOG_ERROR("Parameter error, args length(%d) not equal to 1", args.Length());
				break;
			}

			v8::HandleScope handle_scope(args.GetIsolate());
			if (!args[0]->IsString()) {
				LOG_ERROR("CallBackGetAccountInfo, parameter 0 should be a String");
				break;
			}

			v8::String::Utf8Value str(args[0]);
			std::string address(ToCString(str));

			bubi::AccountFrm::pointer account_frm = nullptr;

			auto environment = LedgerManager::Instance().transaction_stack_.top()->environment_;
			if (!environment->GetEntry(address, account_frm))
				break;

			Json::Value json = bubi::Proto2Json(account_frm->GetProtoAccount());
			v8::Local<v8::String> returnvalue = v8::String::NewFromUtf8(
				args.GetIsolate(), json.toFastString().c_str(), v8::NewStringType::kNormal).ToLocalChecked();
			args.GetReturnValue().Set(v8::JSON::Parse(returnvalue));

			return;
		} while (false);
		args.GetReturnValue().Set(false);
	}

	void ContractManager::CallBackDoOperation(const v8::FunctionCallbackInfo<v8::Value>& args) {


		do {
			if (args.Length() != 1) {
				LOG_ERROR("Parameter error, args length(%d) not equal to 1", args.Length());
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());

			v8::String::Utf8Value token(args.GetIsolate()->GetCurrentContext()->GetSecurityToken()->ToString());
			std::string contractor(ToCString(token));

			if (!args[0]->IsObject()) {
				LOG_ERROR("CallBackDoOperation, parameter 0 should not be null");
				break;
			}

			v8::Local<v8::Object> obj = args[0]->ToObject();
			auto str = v8::JSON::Stringify(args.GetIsolate()->GetCurrentContext(), obj).ToLocalChecked();

			//v8::Local<v8::String> str = v8::JSON::Stringify(args.GetIsolate()->GetCurrentContext()/*context*/, obj).ToLocalChecked();
			v8::String::Utf8Value utf8value(str);
			const char* strdata = ToCString(utf8value);
			Json::Value transaction_json;

			if (!transaction_json.fromCString(strdata)){
				LOG_ERROR("string to json failed, string=%s", strdata);
				break;
			}

			protocol::Transaction transaction;
			std::string error_msg;
			if (!Json2Proto(transaction_json, transaction, error_msg)){
				LOG_ERROR("json to protocol object failed: json=%s. error=%s", strdata, error_msg.c_str());
				break;
			}

			transaction.set_source_address(contractor);

			for (int i = 0; i < transaction.operations_size(); i++){
				protocol::Operation*  ope = transaction.mutable_operations(i);
				ope->set_source_address(contractor);
			}

			//transaction.set_nonce(contract_account->GetAccountNonce());			
			protocol::TransactionEnv env;
			env.mutable_transaction()->CopyFrom(transaction);

			if (!LedgerManager::Instance().DoTransaction(env)){
				break;
			}

			args.GetReturnValue().Set(true);

			return;
		} while (false);

		args.GetReturnValue().Set(false);

	}

	void ContractManager::CallBackLog(const v8::FunctionCallbackInfo<v8::Value>& args) {

		if (args.Length() != 1) {
			LOG_ERROR("Parameter error, args length(%d) not equal to 1", args.Length());
			args.GetReturnValue().Set(false);
			return;
		}
		v8::HandleScope scope(args.GetIsolate());

		v8::String::Utf8Value token(args.GetIsolate()->GetCurrentContext()->GetSecurityToken()->ToString());

		auto context = args.GetIsolate()->GetCurrentContext();
		auto sender = args.GetIsolate()->GetCurrentContext()->Global()->Get(context,
			v8::String::NewFromUtf8(args.GetIsolate(), sender_name_.c_str(), v8::NewStringType::kNormal).ToLocalChecked()).ToLocalChecked();
		v8::String::Utf8Value utf8_sender(sender->ToString());

		v8::Local<v8::String> str;
		if (args[0]->IsObject()) {
			v8::Local<v8::Object> obj = args[0]->ToObject(args.GetIsolate());
			str = v8::JSON::Stringify(args.GetIsolate()->GetCurrentContext(), obj).ToLocalChecked();
		}
		else {
			str = args[0]->ToString();
		}

		auto type = args[0]->TypeOf(args.GetIsolate());
		if (v8::String::NewFromUtf8(args.GetIsolate(), "undefined", v8::NewStringType::kNormal).ToLocalChecked()->Equals(type)) {
			LOG_INFO("LogCallBack[%s:%s]\n%s", ToCString(token), ToCString(utf8_sender), "undefined");
			args.GetReturnValue().Set(true);
			return;
		}

		//
		v8::String::Utf8Value utf8value(str);
		const char* sender_addr = ToCString(utf8_sender);
		const char* log_data = ToCString(utf8value);
		LOG_INFO("LogCallBack[%s:%s]\n%s", ToCString(token), sender_addr, log_data);
		args.GetReturnValue().Set(true);
	}

	//
	void ContractManager::CallBackGetTransactionInfo(const v8::FunctionCallbackInfo<v8::Value>& args) {
		if (args.Length() != 1) {
			LOG_ERROR("Parameter error, args length(%d) not equal to 1", args.Length());
			args.GetReturnValue().Set(false);
			return;
		}

		v8::HandleScope handle_scope(args.GetIsolate());
		if (!args[0]->IsString()) {
			args.GetReturnValue().Set(false);
			LOG_ERROR("CallBackGetTransactionInfo, parameter 0 should not be null");
			return;
		}

		v8::String::Utf8Value str(args[0]);
		std::string hash(ToCString(str));
		bubi::TransactionFrm txfrm;
		std::string hashBin = utils::String::HexStringToBin(hash);
		if (protocol::ERRCODE_SUCCESS == txfrm.LoadFromDb(hashBin)){
			Json::Value json = bubi::Proto2Json(txfrm.GetProtoTxEnv());
			std::string strvalue = json.toStyledString();
			v8::Local<v8::String> returnvalue = v8::String::NewFromUtf8(
				args.GetIsolate(), strvalue.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
			args.GetReturnValue().Set(v8::JSON::Parse(returnvalue));
		}
		else{
			args.GetReturnValue().Set(false);
		}
	}

	//
	void ContractManager::CallBackGetLedgerInfo(const v8::FunctionCallbackInfo<v8::Value>& args) {
		if (args.Length() != 1) {
			LOG_ERROR("Parameter error, args length(%d) not equal to 1", args.Length());
			args.GetReturnValue().Set(false);
			return;
		}

		v8::HandleScope handle_scope(args.GetIsolate());
		if (!args[0]->IsString() && !args[0]->IsNumber()) {
			args.GetReturnValue().Set(false);
			LOG_ERROR("CallBackGetLedgerInfo, parameter 0 should not be null");
			return;
		}

		v8::String::Utf8Value str(args[0]);
		std::string key(ToCString(str));

		int64_t seq = utils::String::Stoi64(key);
		protocol::LedgerHeader lcl = LedgerManager::Instance().GetLastClosedLedger();
		if (seq <= lcl.seq() - 1024 || seq > lcl.seq()) {
			args.GetReturnValue().Set(false);
			LOG_ERROR("The parameter seq(" FMT_I64 ") <= " FMT_I64 " or > " FMT_I64, seq, lcl.seq() - 1024, lcl.seq());
			return;
		}

		LedgerFrm lfrm;
		if (lfrm.LoadFromDb(seq)){

			std::string strvalue = bubi::Proto2Json(lfrm.GetProtoHeader()).toStyledString();
			v8::Local<v8::String> returnvalue = v8::String::NewFromUtf8(
				args.GetIsolate(), strvalue.c_str(), v8::NewStringType::kNormal).ToLocalChecked();

			args.GetReturnValue().Set(v8::JSON::Parse(returnvalue));
		}
		else{
			args.GetReturnValue().Set(false);
		}
	}

	ContractManager* ContractManager::UnwrapContract(v8::Local<v8::Object> obj){
		v8::Local<v8::External> field = v8::Local<v8::External>::Cast(obj->GetInternalField(0));
		void* ptr = field->Value();
		return static_cast<ContractManager*>(ptr);
	}


	//Add 3002 version
	//str to int64 check
	void ContractManager::CallBackStoI64Check(const v8::FunctionCallbackInfo<v8::Value>& args){
		std::string error_desc;
		do {
			if (args.Length() != 1) {
				error_desc = "Parameter number error";
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString()) {
				error_desc = "Contract execution error, stoI64Check, parameter 0 should be a String or Number";
				break;
			}

			std::string arg0 = ToCString(v8::String::Utf8Value(args[0]));

			int64_t iarg0 = 0;
			if (!utils::String::SafeStoi64(arg0, iarg0)) {
				error_desc = "Contract execution error, stoI64Check, parameter 0 illegal, maybe exceed the limit value of int64.";
				break;
			}

			args.GetReturnValue().Set(true);
			return;
		} while (false);
		args.GetReturnValue().Set(false);
	}

	//Int64 add
	void ContractManager::CallBackInt64Add(const v8::FunctionCallbackInfo<v8::Value>& args){
		std::string error_desc;
		do {
			if (args.Length() != 2) {
				error_desc = "Parameter number error";
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString() && !args[0]->IsNumber()) {
				error_desc = "Contract execution error, int64Add, parameter 0 should be a String or Number";
				break;
			}
			if (!args[1]->IsString() && !args[1]->IsNumber()) {
				error_desc = "Contract execution error, int64Add, parameter 1 should be a String or Number";
				break;
			}

			std::string arg0 = ToCString(v8::String::Utf8Value(args[0]));
			std::string arg1 = ToCString(v8::String::Utf8Value(args[1]));
			int64_t iarg0 = 0;
			int64_t iarg1 = 0;

			if (!utils::String::SafeStoi64(arg0, iarg0)) {
				error_desc = "Contract execution error, int64Add, parameter 0 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (!utils::String::SafeStoi64(arg1, iarg1)) {
				error_desc = "Contract execution error, int64Add, parameter 1 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (!utils::SafeIntAdd(iarg0, iarg1, iarg0)) {
				error_desc = "Contract execution error, int64Add, parameter 0 + parameter 1 overflowed";
				break;
			}

			args.GetReturnValue().Set(v8::String::NewFromUtf8(
				args.GetIsolate(), utils::String::ToString(iarg0).c_str(), v8::NewStringType::kNormal).ToLocalChecked());
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	//Int64 sub
	void ContractManager::CallBackInt64Sub(const v8::FunctionCallbackInfo<v8::Value>& args){
		std::string error_desc;
		do {
			if (args.Length() != 2) {
				error_desc = "Parameter number error";
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString() && !args[0]->IsNumber()) {
				error_desc = "Contract execution error, int64Sub, parameter 0 should be a String or Number";
				break;
			}
			if (!args[1]->IsString() && !args[1]->IsNumber()) {
				error_desc = "Contract execution error, int64Sub, parameter 1 should be a String or Number";
				break;
			}

			std::string arg0 = ToCString(v8::String::Utf8Value(args[0]));
			std::string arg1 = ToCString(v8::String::Utf8Value(args[1]));
			int64_t iarg0 = 0;
			int64_t iarg1 = 0;

			if (!utils::String::SafeStoi64(arg0, iarg0)) {
				error_desc = "Contract execution error, int64Sub, parameter 0 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (!utils::String::SafeStoi64(arg1, iarg1)) {
				error_desc = "Contract execution error, int64Sub, parameter 1 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (!utils::SafeIntSub(iarg0, iarg1, iarg0)) {
				error_desc = "Contract execution error, int64Sub, parameter 0 - parameter 1 overflowed";
				break;
			}

			args.GetReturnValue().Set(v8::String::NewFromUtf8(
				args.GetIsolate(), utils::String::ToString(iarg0).c_str(), v8::NewStringType::kNormal).ToLocalChecked());
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	//Int64 mul
	void ContractManager::CallBackInt64Mul(const v8::FunctionCallbackInfo<v8::Value>& args){
		std::string error_desc;
		do {
			if (args.Length() != 2) {
				error_desc = "Parameter number error";
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString() && !args[0]->IsNumber()) {
				error_desc = "Contract execution error, int64Mul, parameter 0 should be a String or Number";
				break;
			}
			if (!args[1]->IsString() && !args[1]->IsNumber()) {
				error_desc = "Contract execution error, int64Mul, parameter 1 should be a String or Number";
				break;
			}

			std::string arg0 = ToCString(v8::String::Utf8Value(args[0]));
			std::string arg1 = ToCString(v8::String::Utf8Value(args[1]));
			int64_t iarg0 = 0;
			int64_t iarg1 = 0;

			if (!utils::String::SafeStoi64(arg0, iarg0)) {
				error_desc = "Contract execution error, int64Mul, parameter 0 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (!utils::String::SafeStoi64(arg1, iarg1)) {
				error_desc = "Contract execution error, int64Mul, parameter 1 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (!utils::SafeIntMul(iarg0, iarg1, iarg0)) {
				error_desc = "Contract execution error, int64Mul, parameter 0 * parameter 1 overflowed";
				break;
			}

			args.GetReturnValue().Set(v8::String::NewFromUtf8(
				args.GetIsolate(), utils::String::ToString(iarg0).c_str(), v8::NewStringType::kNormal).ToLocalChecked());
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	//Int64 mod
	void ContractManager::CallBackInt64Mod(const v8::FunctionCallbackInfo<v8::Value>& args){
		std::string error_desc;
		do {
			if (args.Length() != 2) {
				error_desc = "Parameter number error";
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString() && !args[0]->IsNumber()) {
				error_desc = "Contract execution error, int64Mod, parameter 0 should be a String or Number";
				break;
			}
			if (!args[1]->IsString() && !args[1]->IsNumber()) {
				error_desc = "Contract execution error, int64Mod, parameter 1 should be a String or Number";
				break;
			}

			std::string arg0 = ToCString(v8::String::Utf8Value(args[0]));
			std::string arg1 = ToCString(v8::String::Utf8Value(args[1]));
			int64_t iarg0 = 0;
			int64_t iarg1 = 0;

			if (!utils::String::SafeStoi64(arg0, iarg0)) {
				error_desc = "Contract execution error, int64Mod, parameter 0 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (!utils::String::SafeStoi64(arg1, iarg1)) {
				error_desc = "Contract execution error, int64Mod, parameter 1 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (iarg1 <= 0 || iarg0 < 0) {
				error_desc = "Parameter arg <= 0";
				break;
			}

			args.GetReturnValue().Set(v8::String::NewFromUtf8(
				args.GetIsolate(), utils::String::ToString(iarg0 % iarg1).c_str(), v8::NewStringType::kNormal).ToLocalChecked());
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	//Int64 div
	void ContractManager::CallBackInt64Div(const v8::FunctionCallbackInfo<v8::Value>& args){
		std::string error_desc;
		do {
			if (args.Length() != 2) {
				error_desc = "Parameter number error";
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString() && !args[0]->IsNumber()) {
				error_desc = "Contract execution error, int64Div, parameter 0 should be a String or Number";
				break;
			}
			if (!args[1]->IsString() && !args[1]->IsNumber()) {
				error_desc = "Contract execution error, int64Div, parameter 1 should be a String or Number";
				break;
			}

			std::string arg0 = ToCString(v8::String::Utf8Value(args[0]));
			std::string arg1 = ToCString(v8::String::Utf8Value(args[1]));
			int64_t iarg0 = 0;
			int64_t iarg1 = 0;

			if (!utils::String::SafeStoi64(arg0, iarg0)) {
				error_desc = "Contract execution error, int64Div, parameter 0 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (!utils::String::SafeStoi64(arg1, iarg1)) {
				error_desc = "Contract execution error, int64Div, parameter 1 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (iarg1 <= 0 || iarg0 < 0) {
				error_desc = "Parameter arg <= 0";
				break;
			}

			args.GetReturnValue().Set(v8::String::NewFromUtf8(
				args.GetIsolate(), utils::String::ToString(iarg0 / iarg1).c_str(), v8::NewStringType::kNormal).ToLocalChecked());
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	//Int64 compare
	void ContractManager::CallBackInt64Compare(const v8::FunctionCallbackInfo<v8::Value>& args){
		std::string error_desc;
		do {
			if (args.Length() != 2) {
				error_desc = " Parameter number error";
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString() && !args[0]->IsNumber()) {
				error_desc = "Contract execution error, int64Compare, parameter 0 should be a String or Number";
				break;
			}
			if (!args[1]->IsString() && !args[1]->IsNumber()) {
				error_desc = "Contract execution error, int64Compare, parameter 1 should be a String or Number";
				break;
			}

			std::string arg0 = ToCString(v8::String::Utf8Value(args[0]));
			std::string arg1 = ToCString(v8::String::Utf8Value(args[1]));
			int64_t iarg0 = 0;
			int64_t iarg1 = 0;

			if (!utils::String::SafeStoi64(arg0, iarg0)) {
				error_desc = "Contract execution error, int64Compare, parameter 0 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (!utils::String::SafeStoi64(arg1, iarg1)) {
				error_desc = "Contract execution error, int64Compare, parameter 1 illegal, maybe exceed the limit value of int64.";
				break;
			}

			int32_t compare1 = 0;
			if (iarg0 > iarg1) compare1 = 1;
			else if (iarg0 == iarg1) {
				compare1 = 0;
			}
			else {
				compare1 = -1;
			}

			args.GetReturnValue().Set(v8::Int32::New(args.GetIsolate(), compare1));
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	//Assert an expression
	void ContractManager::CallBackAssert(const v8::FunctionCallbackInfo<v8::Value>& args){
		std::string error_desc = "Assertion exception occurred";
		do {
			if (args.Length() < 1 || args.Length() > 2) {
				error_desc.append(",parameter nums error");
				break;
			}
			if (!args[0]->IsBoolean()) {
				error_desc.append(",parameter 0 should be boolean");
				break;
			}

			v8::HandleScope scope(args.GetIsolate());
			if (args.Length() == 2) {
				if (!args[1]->IsString()) {
					error_desc.append(",parameter 1 should be string");
					break;
				}
				else {
					v8::String::Utf8Value str1(args[1]);
					error_desc = ToCString(str1);
				}
			}
			if (args[0]->BooleanValue() == false) {
				break;
			}
			args.GetReturnValue().Set(true);
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	//Check address valid
	void ContractManager::CallBackAddressValidCheck(const v8::FunctionCallbackInfo<v8::Value>& args){
		std::string error_desc;
		do {
			if (args.Length() != 1) {
				error_desc = "parameter number error";
				break;
			}

			if (!args[0]->IsString()) {
				error_desc = "arg0 should be string";
				break;
			}

			v8::HandleScope handle_scope(args.GetIsolate());

			v8::String::Utf8Value utf8(args[0]);
			std::string address = std::string(ToCString(utf8));
			bool ret = PublicKey::IsAddressValid(address);

			args.GetReturnValue().Set(ret);
			return;
		} while (false);

		args.GetReturnValue().Set(false);
	}

	//Get the hash of one of the 1024 most recent complete blocks
	void ContractManager::CallBackSha256(const v8::FunctionCallbackInfo<v8::Value>& args){
		do {
			if (args.Length() != 1 && args.Length() != 2) {
				LOG_TRACE("Parameter error");
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());
			if (!args[0]->IsString()) {
				LOG_TRACE("Contract execution error, parameter 0 should be a string.");
				break;
			}
			DataEncodeType encode_type = BASE16;
			if (args.Length() == 2) {
				if (!TransEncodeType(args[1], encode_type)) {
					LOG_TRACE("Contract execution error, trans data encode type wrong.");
					break;
				}
			}

			std::string sha256_data;
			if (!TransEncodeData(args[0], encode_type, sha256_data)) {
				LOG_TRACE("Contract execution error, trans data wrong.");
				break;
			}

			std::string output = utils::Sha256::Crypto(sha256_data);
			if (output.empty()) {
				LOG_TRACE("Sha256 result empty");
				break;
			}

			args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(), utils::String::BinToHexString(output).c_str(),
				v8::NewStringType::kNormal).ToLocalChecked());
			return;
		} while (false);
		args.GetReturnValue().Set(false);
	}

	void ContractManager::CallBackVerify(const v8::FunctionCallbackInfo<v8::Value>& args){
		bool result = false;
		do {
			if (args.Length() != 3 && args.Length() != 4) {
				LOG_TRACE("Parameter error");
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());
			if (!args[0]->IsString() || !args[1]->IsString() || !args[2]->IsString()) {
				LOG_TRACE("Parameters should be string");
				break;
			}

			DataEncodeType encode_type = BASE16;
			if (args.Length() == 4) {
				if (!TransEncodeType(args[3], encode_type)) {
					LOG_TRACE("Contract execution error, trans data encode type wrong.");
					break;
				}
			}

			std::string signed_data = ToCString(v8::String::Utf8Value(args[0]));
			std::string public_key = ToCString(v8::String::Utf8Value(args[1]));
			std::string blob_data;
			if (!TransEncodeData(args[2], encode_type, blob_data)) {
				LOG_TRACE("Contract execution error, trans data wrong.");
				break;
			}

			if (blob_data.empty() || signed_data.empty() || public_key.empty()) {
				LOG_TRACE("Parameter are empty");
				break;
			}

			result = PublicKey::Verify(blob_data, utils::String::HexStringToBin(signed_data), public_key);
		} while (false);
		args.GetReturnValue().Set(result);
	}

	void ContractManager::CallBackToAddress(const v8::FunctionCallbackInfo<v8::Value>& args){
		do {
			if (args.Length() != 1) {
				LOG_TRACE("Parameter error");
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());
			if (!args[0]->IsString()) {
				LOG_TRACE("Contract execution error, parameter 0 should be a string");
				break;
			}

			std::string pub_key_str = ToCString(v8::String::Utf8Value(args[0]));
			if (pub_key_str.empty()) {
				LOG_TRACE("To address parameter empty");
				break;
			}

			bubi::PublicKey pub_key(pub_key_str);
			if (!pub_key.IsValid()) {
				LOG_TRACE("ConvertPublicKey public key invalid.%s", pub_key_str.c_str());
				break;
			}
			args.GetReturnValue().Set(v8::String::NewFromUtf8(
				args.GetIsolate(), pub_key.GetBase16Address().c_str(), v8::NewStringType::kNormal).ToLocalChecked());

			return;
		} while (false);
		args.GetReturnValue().Set(false);
	}


	bool ContractManager::TransEncodeType(const v8::Local<v8::Value> &arg, DataEncodeType &data_type) {
		if (!arg->IsNumber()) {
			LOG_TRACE("Contract execution error, parameter should be a number.");
			return false;
		}

		std::string arg_str = ToCString(v8::String::Utf8Value(arg));
		int64_t arg_num = 0;
		if (!utils::String::SafeStoi64(arg_str, arg_num)) {
			LOG_TRACE("Contract execution error, encode type maybe exceed the limit value of int64.");
			return false;
		}
		if (arg_num < 0 || arg_num > BASE64) {
			LOG_TRACE("Contract execution error, encode type must be in 0-2");
			return false;
		}
		data_type = (DataEncodeType)arg_num;
		return true;
	}

	bool ContractManager::TransEncodeData(const v8::Local<v8::Value> &raw_data, const DataEncodeType &encode_type, std::string &result_data) {
		result_data.clear();
		std::string input_raw = ToCString(v8::String::Utf8Value(raw_data));
		switch (encode_type) {
		case BASE16:{
						result_data = utils::String::HexStringToBin(input_raw);
						break;
		}
		case RAW_DATA:{
						  result_data = input_raw;
						  break;
		}
		case BASE64:{
						utils::Base64Decode(input_raw, result_data);
						break;
		}
		default:
			break;
		}

		if (result_data.empty()) {
			LOG_TRACE("TransEncodeData error");
			return false;
		}
		return true;
	}


	//bool ContractManager::DoTransaction(protocol::TransactionEnv& env){
	//	auto back = LedgerManager::Instance().transaction_stack_.second;
	//	std::shared_ptr<AccountFrm> source_account;
	//	back->environment_->GetEntry(env.transaction().source_address(), source_account);
	//	env.mutable_transaction()->set_nonce(source_account->GetAccountNonce() + 1);
	//	auto txfrm = std::make_shared<bubi::TransactionFrm >(env);
	//	//LedgerManager::Instance().execute_transaction_.second = txfrm;

	//	auto header = std::make_shared<protocol::LedgerHeader>(LedgerManager::Instance().closing_ledger_->GetProtoHeader());

	//	if (txfrm->ValidForParameter()){
	//		txfrm->Apply(header, true);
	//	}

	//	if (txfrm->GetResult().code() == protocol::ERRCODE_SUCCESS){
	//		txfrm->AllCommit();
	//	}

	//	//LedgerManager::Instance().execute_transaction_.second = back;
	//	return txfrm->GetResult().code() == protocol::ERRCODE_SUCCESS;
	//}
}