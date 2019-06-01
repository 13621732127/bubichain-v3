
#include <utils/headers.h>
#include <utils/common.h>
#include <common/general.h>
#include <notary/configure.h>
#include <notary/notary_mgr.h>
#include <common/private_key.h>

namespace bubi {

	ChainObj::ChainObj(MessageChannel *channel, const std::string &comm_unique, const std::string &notary_address, const std::string &private_key){
		chain_info_.Reset();
		peer_chain_ = nullptr;
		channel_ = channel;
		comm_unique_ = comm_unique;
		notary_address_ = notary_address;
		private_key_ = private_key;
	}

	ChainObj::~ChainObj(){

	}

	void ChainObj::OnTimer(int64_t current_time){
		//Get the latest output list and sort outmap
		RequestAndSortOutput();

		//Get the latest intput list and sort the inputmap
		RequestAndSortInput();

		//Check the number of tx errors
		CheckTxError();

		//vote output
		VoteOutPut();

		//vote input
		VoteInPut();
	}

	void ChainObj::OnHandleMessage(const protocol::WsMessage &message){
		if (message.type() == protocol::CROSS_MSGTYPE_PROPOSAL && !message.request()){
			OnHandleProposalResponse(message);
		}

		if (message.type() == protocol::CROSS_MSGTYPE_PROPOSAL_NOTICE){
			OnHandleProposalNotice(message);
		}

		if (message.type() == protocol::CROSS_MSGTYPE_NOTARYS && !message.request()){
			OnHandleNotarysResponse(message);
		}

		if (message.type() == protocol::CROSS_MSGTYPE_ACCOUNT_NONCE && !message.request()){
			OnHandleAccountNonceResponse(message);
		}

		if (message.type() == protocol::CROSS_MSGTYPE_DO_TRANSACTION && !message.request()){
			OnHandleProposalDoTransResponse(message);
		}
	}

	void ChainObj::SetPeerChain(std::shared_ptr<ChainObj> peer_chain){
		peer_chain_ = peer_chain;
	}

	ChainObj::ChainInfo ChainObj::GetChainInfo(){
		utils::MutexGuard guard(lock_);
		return chain_info_;
	}

	void ChainObj::OnHandleProposalNotice(const protocol::WsMessage &message){
		protocol::CrossProposalInfo cross_proposal;
		cross_proposal.ParseFromString(message.data());
		LOG_INFO("Recv Proposal Notice..");
		HandleProposalNotice(cross_proposal);
		return;
	}

	void ChainObj::OnHandleProposalResponse(const protocol::WsMessage &message){
		protocol::CrossProposalResponse msg;
		msg.ParseFromString(message.data());
		LOG_INFO("Recv Proposal Response..");
		HandleProposalNotice(msg.proposal_info());
	}

	void ChainObj::OnHandleNotarysResponse(const protocol::WsMessage &message){
		protocol::CrossNotarysResponse msg;
		msg.ParseFromString(message.data());
		LOG_INFO("Recv Notarys Response..");

		if (msg.notarys_size() >= 100){
			LOG_ERROR("Notary nums is no more than 100");
			return;
		}

		for (int i = 0; i < msg.notarys_size(); i++) {
			chain_info_.notary_list[i] = msg.notarys(i);
		}
	}

	void ChainObj::OnHandleAccountNonceResponse(const protocol::WsMessage &message){
		protocol::CrossAccountNonceResponse msg;
		msg.ParseFromString(message.data());
		LOG_INFO("Recv Account Nonce Response..");
		chain_info_.nonce = msg.nonce();
	}

	void ChainObj::OnHandleProposalDoTransResponse(const protocol::WsMessage &message){
		protocol::CrossDoTransactionResponse msg;
		msg.ParseFromString(message.data());
		LOG_INFO("Recv Do Trans Response..");

		auto iter = std::find(chain_info_.tx_history.begin(), chain_info_.tx_history.end(), msg.hash());
		if (iter == chain_info_.tx_history.end()){
			return;
		}

		if (msg.error_code() == protocol::ERRCODE_SUCCESS){
			chain_info_.error_tx_times = 0;
			chain_info_.tx_history.clear();
			return;
		}
		chain_info_.error_tx_times++;
		LOG_ERROR("Failed to Do Transaction, (" FMT_I64 "),tx hash is %s,err_code is (" FMT_I64 "),err_desc is %s",
			msg.hash().c_str(), msg.error_code(), msg.error_desc().c_str());

	}

	void ChainObj::HandleProposalNotice(const protocol::CrossProposalInfo &proposal_info){
		//save proposal
		utils::MutexGuard guard(lock_);
		if (proposal_info.type() == protocol::CROSS_PROPOSAL_OUTPUT){
			chain_info_.output_map[proposal_info.proposal_id()] = proposal_info;
			chain_info_.output_recv_max_seq = MAX(proposal_info.proposal_id(), chain_info_.output_recv_max_seq);
			if (proposal_info.status() == "ok"){
				chain_info_.output_affirm_max_seq = MAX(proposal_info.proposal_id(), chain_info_.output_affirm_max_seq);
			}

		}
		else if (proposal_info.type() == protocol::CROSS_PROPOSAL_INPUT){
			chain_info_.input_map[proposal_info.proposal_id()] = proposal_info;
			chain_info_.input_recv_max_seq = MAX(proposal_info.proposal_id(), chain_info_.input_recv_max_seq);
			if (proposal_info.status() == "ok"){
				chain_info_.input_affirm_max_seq = MAX(proposal_info.proposal_id(), chain_info_.input_affirm_max_seq);
			}
		}
		else{
			LOG_ERROR("Unknown proposal type.");
		}
	}

	void ChainObj::RequestAndSortOutput(){
		utils::MutexGuard guard(lock_);
		//�������µ�output�᰸�����ȷ�ϵ�output�᰸
		protocol::CrossProposal proposal;
		proposal.set_type(protocol::CROSS_PROPOSAL_OUTPUT);
		proposal.set_proposal_id(-1);
		channel_->SendRequest(comm_unique_, protocol::CROSS_MSGTYPE_PROPOSAL, proposal.SerializeAsString());

		CrossProposalInfoMap &out_map = chain_info_.output_map;
		//ɾ������ɵ��᰸
		for (auto itr = out_map.begin(); itr != out_map.end();){
			const protocol::CrossProposalInfo &info = itr->second;
			if (info.status() != "ok"){
				itr++;
			}

			out_map.erase(itr++);
		}
		
		//��������ȷʵ��ʼ���ȱʧoutput���᰸
		int64_t max_nums = MIN(100, (chain_info_.output_recv_max_seq - chain_info_.output_affirm_max_seq));
		if (max_nums <= 0){
			max_nums = 1;
		}
		for (int64_t i = 1; i <= max_nums; i++){
			int64_t index = chain_info_.output_affirm_max_seq + i;
			auto itr = chain_info_.output_map.find(index);
			if (itr != chain_info_.output_map.end()){
				continue;
			}
			protocol::CrossProposal proposal;
			proposal.set_type(protocol::CROSS_PROPOSAL_OUTPUT);
			proposal.set_proposal_id(index);
			channel_->SendRequest(comm_unique_, protocol::CROSS_MSGTYPE_PROPOSAL, proposal.SerializeAsString());
		}
	}

	void ChainObj::RequestAndSortInput(){
		utils::MutexGuard guard(lock_);
		//�������µ�input�᰸
		protocol::CrossProposal proposal;
		proposal.set_type(protocol::CROSS_PROPOSAL_INPUT);
		proposal.set_proposal_id(-1);
		channel_->SendRequest(comm_unique_, protocol::CROSS_MSGTYPE_PROPOSAL, proposal.SerializeAsString());

		CrossProposalInfoMap &input_map = chain_info_.input_map;
		//ɾ������ɵ��᰸
		for (auto itr = input_map.begin(); itr != input_map.end();){
			const protocol::CrossProposalInfo &info = itr->second;
			if (info.status() != "ok"){
				itr++;
			}

			input_map.erase(itr++);
		}

		//��������ȷʵ��ʼ���ȱʧinput���᰸
		int64_t max_nums = MIN(100, (chain_info_.input_recv_max_seq - chain_info_.input_affirm_max_seq));
		if (max_nums <= 0){
			max_nums = 1;
		}
		for (int64_t i = 1; i <= max_nums; i++){
			int64_t index = chain_info_.input_affirm_max_seq + i;
			auto itr = chain_info_.input_map.find(index);
			if (itr != chain_info_.input_map.end()){
				continue;
			}
			protocol::CrossProposal proposal;
			proposal.set_type(protocol::CROSS_PROPOSAL_INPUT);
			proposal.set_proposal_id(index);
			channel_->SendRequest(comm_unique_, protocol::CROSS_MSGTYPE_PROPOSAL, proposal.SerializeAsString());
		}
	}

	void ChainObj::CheckTxError(){
		//TODO �����쳣�Ľ���
	}

	void ChainObj::VoteOutPut(){
		//����Լ���output�б����һ�����״̬����һ��ֵ�Ƿ����
		if (chain_info_.output_affirm_max_seq < 0){
			LOG_ERROR("No output affirm max seq.");
			return;
		}

		protocol::CrossProposalInfo vote_proposal;
		vote_proposal.set_proposal_id(-1);
		do 
		{
			utils::MutexGuard guard(lock_);
			const CrossProposalInfoMap &out_map = chain_info_.output_map;
			auto itr = out_map.find(chain_info_.output_affirm_max_seq + 1);
			if (itr != out_map.end()){
				LOG_INFO("No output unaffirmed proposal.");
				//��������ڼ��Զ˵�intput�б��Ƿ���Ҫ�����µ�ͶƱ���
				//TODO GetPeerInput Proposal
				vote_proposal;
				break;
			}

			//����������ж��Լ��Ƿ�Ͷ��Ʊ������ͶƱ����
			const protocol::CrossProposalInfo &proposal = itr->second;
			bool confirmed = false;
			for (size_t i = 0; i < proposal.confirmed_notarys_size(); i++){
				if (proposal.confirmed_notarys(i) != notary_address_){
					confirmed = true;
					break;
				}
			}

			if (confirmed){
				break;
			}
			vote_proposal = proposal;
		} while (false);

		
		if (vote_proposal.proposal_id() == -1){
			return;
		}

		//�����ף��ı��᰸״̬Ϊoutput��ʽ
		vote_proposal.set_type(protocol::CROSS_PROPOSAL_OUTPUT);
		SubmitTransaction(vote_proposal);
	}

	void ChainObj::VoteInPut(){
		//����Լ���input�б����һ�����״̬����һ��ֵ�Ƿ����
		if (chain_info_.input_affirm_max_seq < 0){
			LOG_ERROR("No input affirm max seq.");
			return;
		}

		protocol::CrossProposalInfo vote_proposal;
		vote_proposal.set_proposal_id(-1);
		do
		{
			utils::MutexGuard guard(lock_);
			const CrossProposalInfoMap &input_map = chain_info_.input_map;
			auto itr = input_map.find(chain_info_.input_affirm_max_seq + 1);
			if (itr != input_map.end()){
				LOG_INFO("No input_map unaffirmed proposal.");
				//��������ڼ��Զ˵�output�б��Ƿ���Ҫ�����µ�ͶƱ���
				//TODO GetPeerOutput Proposal
				vote_proposal;
				break;
			}

			//����������ж��Լ��Ƿ�Ͷ��Ʊ������ͶƱ����
			const protocol::CrossProposalInfo &proposal = itr->second;
			bool confirmed = false;
			for (size_t i = 0; i < proposal.confirmed_notarys_size(); i++){
				if (proposal.confirmed_notarys(i) != notary_address_){
					confirmed = true;
					break;
				}
			}

			if (confirmed){
				break;
			}
			vote_proposal = proposal;
		} while (false);

		if (vote_proposal.proposal_id() == -1){
			return;
		}

		//�����ף��ı��᰸״̬Ϊinput��ʽ
		vote_proposal.set_type(protocol::CROSS_PROPOSAL_INPUT);
		SubmitTransaction(vote_proposal);
	}

	void ChainObj::SubmitTransaction(const protocol::CrossProposalInfo &vote_proposal){
		protocol::CrossDoTransaction cross_do_trans;
		protocol::TransactionEnv tran_env;
		protocol::Transaction *tran = tran_env.mutable_transaction();
		//TODO ���콻��
		
		std::string content = tran->SerializeAsString();
		PrivateKey privateKey(private_key_);
		if (!privateKey.IsValid()) {
			LOG_ERROR("Submit transaction error.");
			return;
		}
		std::string sign = privateKey.Sign(content);
		protocol::Signature *signpro = tran_env.add_signatures();
		signpro->set_sign_data(sign);
		signpro->set_public_key(privateKey.GetBase16PublicKey());
	}

	NotaryMgr::NotaryMgr(){
	}

	NotaryMgr::~NotaryMgr(){
	}

	bool NotaryMgr::Initialize(){

		NotaryConfigure &config = Configure::Instance().notary_configure_;

		if (!config.enabled_){
			LOG_TRACE("Failed to init notary mgr, configuration file is not allowed");
			return true;
		}
		
		TimerNotify::RegisterModule(this);
		LOG_INFO("Initialized notary mgr successfully");

		PairChainMap::iterator itr = Configure::Instance().pair_chain_map_.begin();
		const PairChainConfigure &config_a = itr->second;
		itr++;
		const PairChainConfigure &config_b = itr->second;
		std::shared_ptr<ChainObj> a = std::make_shared<ChainObj>(&channel_, config_a.comm_unique_, config.address_);
		std::shared_ptr<ChainObj> b = std::make_shared<ChainObj>(&channel_, config_b.comm_unique_, config.address_);

		chain_obj_map_[config_a.comm_unique_] = a;
		chain_obj_map_[config_b.comm_unique_] = b;

		a->SetPeerChain(b);
		b->SetPeerChain(a);

		ChannelParameter param;
		param.inbound_ = true;
		param.notary_addr_ = config.listen_addr_;
		channel_.Initialize(param);
		channel_.Register(this, protocol::CROSS_MSGTYPE_PROPOSAL);
		channel_.Register(this, protocol::CROSS_MSGTYPE_PROPOSAL_NOTICE);
		channel_.Register(this, protocol::CROSS_MSGTYPE_NOTARYS);
		channel_.Register(this, protocol::CROSS_MSGTYPE_ACCOUNT_NONCE);
		channel_.Register(this, protocol::CROSS_MSGTYPE_DO_TRANSACTION);

		return true;
	}

	bool NotaryMgr::Exit(){
		return true;
	}

	void NotaryMgr::OnTimer(int64_t current_time){
		for (auto itr = chain_obj_map_.begin(); itr != chain_obj_map_.end(); itr++){
			itr->second->OnTimer(current_time);
		}
	}

	void NotaryMgr::HandleMessage(const std::string &comm_unique, const protocol::WsMessage &message){
		//Check existing in chain obj
		auto itr = chain_obj_map_.find(comm_unique);
		if (itr == chain_obj_map_.end()){
			LOG_ERROR("Can not find chain.");
			return;
		}
		LOG_INFO("Recv comm unique %s message", comm_unique.c_str());
		//Deliver the message to chain obj
		itr->second->OnHandleMessage(message);
	}
}

