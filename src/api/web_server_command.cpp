#include <utils/headers.h>
#include <common/private_key.h>
#include <common/general.h>
#include <main/configure.h>
#include <overlay/peer_manager.h>
#include <glue/glue_manager.h>
#include <ledger/ledger_manager.h>
#include <ledger/contract_manager.h>
#include "web_server.h"

namespace bubi {

	void WebServer::SubmitTransaction(const http::server::request &request, std::string &reply) {

		Json::Value body;
		if (!body.fromString(request.body)) {
			LOG_ERROR("Parse request body json failed");
			Json::Value reply_json;
			reply_json["results"][Json::UInt(0)]["error_code"] = protocol::ERRCODE_INVALID_PARAMETER;
			reply_json["results"][Json::UInt(0)]["error_desc"] = "request must being json format";
			reply_json["success_count"] = Json::UInt(0);
			reply = reply_json.toStyledString();
			return;
		}

		Json::Value reply_json = Json::Value(Json::objectValue);
		Json::Value &results = reply_json["results"];
		results = Json::Value(Json::arrayValue);
		uint32_t success_count = 0;

		int64_t begin_time = utils::Timestamp::HighResolution();
		const Json::Value &json_items = body["items"];
		for (size_t j = 0; j < json_items.size() && running; j++) {
			const Json::Value &json_item = json_items[j];
			Json::Value &result_item = results[results.size()];

			int64_t active_time = utils::Timestamp::HighResolution();
			Result result;
			result.set_code(protocol::ERRCODE_SUCCESS);
			result.set_desc("");

			protocol::TransactionEnv tran_env;
			do {
				if (json_item.isMember("transaction_blob")) {
					if (!json_item.isMember("signatures")) {
						result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
						result.set_desc("'signatures' value not exist");
						break;
					}

					std::string decodeblob;
					std::string decodesig;
					//utils::decode_b16(json_item["transaction_blob"].asString(), decodeblob);
					decodeblob;// = utils::String::HexStringToBin(json_item["transaction_blob"].asString());
					if (!utils::String::HexStringToBin(json_item["transaction_blob"].asString(), decodeblob)) {
						result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
						result.set_desc("'transaction_blob' value must be Hex");
						break;
					}

					protocol::Transaction *tran = tran_env.mutable_transaction();
					if (!tran->ParseFromString(decodeblob)) {
						result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
						result.set_desc("ParseFromString from 'sign_data' invalid");
						LOG_ERROR("ParseFromString from decodeblob invalid");
						break;
					}

					const Json::Value &signatures = json_item["signatures"];
					for (uint32_t i = 0; i < signatures.size(); i++) {
						const Json::Value &signa = signatures[i];
						protocol::Signature *signpro = tran_env.add_signatures();

						//utils::decode_b16(signa["sign_data"].asString(), decodesig);
						decodesig = "";
						if (!utils::String::HexStringToBin(signa["sign_data"].asString(), decodesig)) {
							result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
							result.set_desc("'sign_data' value must be Hex");
							break;
						}

						PublicKey pubkey(signa["public_key"].asString());
						if (!pubkey.IsValid()) {
							result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
							result.set_desc("'public_key' value not exist or parameter error");
							LOG_ERROR("Invalid publickey (%s)", signa["public_key"].asString().c_str());
							break;
						}

						signpro->set_sign_data(decodesig);
						signpro->set_public_key(pubkey.GetBase16PublicKey());
					}

					// add node signature
					std::string content = tran->SerializeAsString();
					PrivateKey privateKey(bubi::Configure::Instance().p2p_configure_.node_private_key_);
					if (!privateKey.IsValid()) {
						result.set_code(protocol::ERRCODE_INVALID_PRIKEY);
						result.set_desc("signature failed");
						break;
					}
					std::string sign = privateKey.Sign(content);
					protocol::Signature *signpro = tran_env.add_signatures();
					signpro->set_sign_data(sign);
					signpro->set_public_key(privateKey.GetBase16PublicKey());
					result_item["hash"] = utils::String::BinToHexString(HashWrapper::Crypto(content));
				}
				else {
					protocol::Transaction *tran = tran_env.mutable_transaction();
					std::string error_msg;
					if (!Json2Proto(json_item["transaction_json"], *tran, error_msg)){
						result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
						result.set_desc(error_msg);
						break;
					}
	

					std::string content = tran->SerializeAsString();
					const Json::Value &private_keys = json_item["private_keys"];
					for (uint32_t i = 0; i < private_keys.size(); i++) {
						const std::string &private_key = private_keys[i].asString();

						PrivateKey privateKey(private_key);
						if (!privateKey.IsValid()) {
							result.set_code(protocol::ERRCODE_INVALID_PRIKEY);
							result.set_desc("signature failed");
							break;
						}

						std::string sign = privateKey.Sign(content);
						protocol::Signature *signpro = tran_env.add_signatures();
						signpro->set_sign_data(sign);
						signpro->set_public_key(privateKey.GetBase16PublicKey());
					}

					// add node signature
					PrivateKey privateKey(bubi::Configure::Instance().p2p_configure_.node_private_key_);
					if (!privateKey.IsValid()) {
						result.set_code(protocol::ERRCODE_INVALID_PRIKEY);
						result.set_desc("signature failed");
						break;
					}
					std::string sign = privateKey.Sign(content);
					protocol::Signature *signpro = tran_env.add_signatures();
					signpro->set_sign_data(sign);
					signpro->set_public_key(privateKey.GetBase16PublicKey());
					result_item["hash"] = utils::String::BinToHexString(HashWrapper::Crypto(content));
				}

				TransactionFrm frm(tran_env);
				if (!frm.CheckValid(-1)){
					result = frm.result_;
					break;
				}

			} while (false);

			if (result.code() == protocol::ERRCODE_SUCCESS) {
				TransactionFrm::pointer ptr = std::make_shared<TransactionFrm>(tran_env);
				GlueManager::Instance().OnTransaction(ptr, result);
				PeerManager::Instance().Broadcast(protocol::OVERLAY_MSGTYPE_TRANSACTION, tran_env.SerializeAsString());
				if (result.code() == protocol::ERRCODE_SUCCESS) success_count++;
			}
			result_item["error_code"] = result.code();
			result_item["error_desc"] = result.desc();
		}
		LOG_TRACE("Create %u transaction use " FMT_I64 "(ms)", json_items.size(),
			(utils::Timestamp::HighResolution() - begin_time) / utils::MICRO_UNITS_PER_MILLI);


		reply_json["success_count"] = success_count;
		reply = reply_json.toStyledString();
	}

	void WebServer::CreateAccount(const http::server::request &request, std::string &reply) {
		std::string error_desc;
		int32_t error_code = protocol::ERRCODE_SUCCESS;
		Json::Value reply_json = Json::Value(Json::objectValue);
		
		SignatureType sign_type = SIGNTYPE_CFCASM2;

		do {
			std::string sign_type_s = request.GetParamValue("sign_type");
			if (sign_type_s == ""){
				if (HashWrapper::GetLedgerHashType() == HashWrapper::HASH_TYPE_SHA256) {
					sign_type = SIGNTYPE_ED25519;
				}
				else {
					sign_type = SIGNTYPE_CFCASM2;
				}
			}
			else {
				sign_type = GetSignTypeByDesc(sign_type_s);
				if (sign_type == SIGNTYPE_NONE) {
					error_code = protocol::ERRCODE_INVALID_PARAMETER;
					break;
				} 
			}

			PrivateKey priv_key(sign_type);
			std::string public_key = priv_key.GetBase16PublicKey();
			std::string private_key = priv_key.GetBase16PrivateKey();
			std::string public_address = priv_key.GetBase16Address();

			LOG_TRACE("Creating account address:%s", public_address.c_str());

			Json::Value &result = reply_json["result"];
			result["public_key"] = public_key;
			result["private_key"] = private_key;
			result["private_key_aes"] = utils::Aes::CryptoHex(private_key, bubi::GetDataSecuretKey());
			result["address"] = public_address;
			result["public_key_raw"] = utils::String::BinToHexString(priv_key.GetRawPublicKey());
			result["sign_type"] = GetSignTypeDesc(sign_type);

		} while (false);
		reply_json["error_code"] = error_code;
		reply = reply_json.toStyledString();
	}

	void WebServer::ConfValidator(const http::server::request &request, std::string &reply) {
		std::string error_desc;
		int32_t error_code = protocol::ERRCODE_SUCCESS;
		Json::Value reply_json = Json::Value(Json::objectValue);

		do {
			// if validator_conf_key is empty, limit local url, otherwise limit validator_conf_key
			if (!request.body.empty()) {
				Json::Value body;
				if (!body.fromString(request.body)) {
					LOG_ERROR("Parse request body json failed");
					error_code = protocol::ERRCODE_ACCESS_DENIED;
					error_desc = "request must being json format";
					break;
				}
				std::string validator_conf_key = body.fromString(request.body) ? body["validator_conf_key"].asString() : "";
				int64_t timestamp = body.isMember("timestamp") ? (body["timestamp"].isString() ? -2 : body["timestamp"].asUInt64()) : -1;
				if (validator_conf_key.empty()) {
					error_code = protocol::ERRCODE_ACCESS_DENIED;
					error_desc = "The validator_conf_key cannot be empty";
					break;
				}
				if (-2 == timestamp) {
					error_code = protocol::ERRCODE_ACCESS_DENIED;
					error_desc = "The timestamp must be number";
					break;
				}
				else if (-1 == timestamp) {
					error_code = protocol::ERRCODE_ACCESS_DENIED;
					error_desc = "The timestamp cannot be empty";
					break;
				}
				else if (utils::Timestamp::HighResolution() - timestamp * 1000 > utils::SECOND_UNITS_PER_DAY * utils::MICRO_UNITS_PER_SEC ||
					timestamp * 1000 - utils::Timestamp::HighResolution() > utils::SECOND_UNITS_PER_DAY * utils::MICRO_UNITS_PER_SEC) {
					error_code = protocol::ERRCODE_ACCESS_DENIED;
					error_desc = "The timestamp was wrong, please check";
					break;
				}

				std::string code_time = Configure::Instance().webserver_configure_.validator_conf_key + utils::String::Format("%lld", timestamp);
				std::string code_hash = utils::String::BinToHexString(utils::Sha256::Crypto(code_time));
				if (code_hash.compare(validator_conf_key) != 0) {
					error_code = protocol::ERRCODE_ACCESS_DENIED;
					error_desc = "This validator_conf_key was wrong ,please check!";
					break;
				}
			}
			else if (!request.peer_address_.IsLoopback()) {
				error_code = protocol::ERRCODE_ACCESS_DENIED;
				error_desc = "This url should be called from local";
				break;
			}

			std::string add = request.GetParamValue("add");
			std::string del = request.GetParamValue("del");

			Result ret = GlueManager::Instance().ConfValidator(add, del);
			error_code = ret.code();
			error_desc = ret.desc();
		} while (false);

		reply_json["error_code"] = error_code;
		reply_json["error_desc"] = error_desc;
		reply = reply_json.toStyledString();
	}

	void WebServer::TestContract(const http::server::request &request, std::string &reply) {
		
		Json::Value body;
		if (!body.fromString(request.body)) {
			LOG_ERROR("Parse request body json failed");
			Json::Value reply_json;
			reply_json["results"][Json::UInt(0)]["error_code"] = protocol::ERRCODE_INVALID_PARAMETER;
			reply_json["results"][Json::UInt(0)]["error_desc"] = "request must being json format";
			reply_json["success_count"] = Json::UInt(0);
			reply = reply_json.toStyledString();
			return;
		}

		ContractTestParameter test_parameter;
		test_parameter.code_ = body["code"].asString();
		test_parameter.input_ = body["input"].asString();
		test_parameter.exe_or_query_ = body["exe_or_query"].asBool();
		test_parameter.contract_address_ = body["contract_address"].asString();
		test_parameter.source_address_ = body["source_address"].asString();

		int32_t error_code = protocol::ERRCODE_SUCCESS;
		std::string error_desc;
		AccountFrm::pointer acc = NULL;

		Json::Value reply_json = Json::Value(Json::objectValue);
		Json::Value &result = reply_json["result"];

		do {
			if (!test_parameter.contract_address_.empty()) {
				if (!Environment::AccountFromDB(test_parameter.contract_address_, acc)) {
					error_code = protocol::ERRCODE_NOT_EXIST;
					error_desc = utils::String::Format("Account(%s) not exist", test_parameter.contract_address_.c_str());
					LOG_ERROR("%s", error_desc.c_str());
					break;
				}

				std::string code = acc->GetProtoAccount().contract().payload();
				if (code.empty()) {
					error_code = protocol::ERRCODE_NOT_EXIST;
					error_desc = utils::String::Format("Account(%s) has no contract code", test_parameter.contract_address_.c_str());
					LOG_ERROR("%s", error_desc.c_str());
					break;
				}
			} 

			Result exe_result;
			if (!LedgerManager::Instance().context_manager_.SyncTestProcess(Contract::TYPE_V8, 
				test_parameter, 
				utils::MICRO_UNITS_PER_SEC, 
				exe_result, result["logs"], result["txs"], result["rets"])) {
				error_code = exe_result.code();
				error_desc = exe_result.desc();
				LOG_ERROR("%s", error_desc.c_str());
				break;
			}

		} while (false);

		reply_json["error_code"] = error_code;
		reply_json["error_desc"] = error_desc;
		reply = reply_json.toStyledString();
	}
}