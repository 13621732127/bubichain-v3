
#include <utils/headers.h>
#include <common/general.h>
#include <notary/configure.h>
#include <notary/notary_mgr.h>

namespace bubi {

	ChainObj::ChainObj(){
		peer_chain_ = nullptr;
	}

	ChainObj::~ChainObj(){

	}

	void ChainObj::OnTimer(int64_t current_time){
		//��ȡ���µ�output�б�

		//��ȡ���µ�intput�б�

		//����outmap

		//����inputmap

		//��齻�״�������Ƿ񳬹����ֵ

		//ͶƱoutput

		//ͶƱinput
	}

	void ChainObj::SetChainInfo(const std::string &comm_unique, const std::string &target_comm_unique){
		comm_unique_ = comm_unique;
		target_comm_unique_ = target_comm_unique;
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

	void ChainObj::SetPeerChain(ChainObj *peer_chain){
		peer_chain_ = peer_chain;
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
		protocol::CrossNotarys msg;
		msg.ParseFromString(message.data());
		LOG_INFO("Recv Notarys Response..");
		//TODO ����֤���б�

	}

	void ChainObj::OnHandleAccountNonceResponse(const protocol::WsMessage &message){
		protocol::CrossAccountNonceResponse msg;
		msg.ParseFromString(message.data());
		LOG_INFO("Recv Account Nonce Response..");
		//TODO �����˺�nonceֵ

	}

	void ChainObj::OnHandleProposalDoTransResponse(const protocol::WsMessage &message){
		protocol::CrossDoTransactionResponse msg;
		msg.ParseFromString(message.data());
		LOG_INFO("Recv Do Trans Response..");
		//TODO �����׽��ֵ

	}

	void ChainObj::HandleProposalNotice(const protocol::CrossProposalInfo &proposal_info){
		//TODO �����᰸����
		LOG_INFO("Handel Proposal Notice..");
	}

	void ChainObj::VoteOutPut(){
		//1.��ȡ�Զ�input�б�����

		//2.��ȡ�Լ���output�б�

		//3.����Լ���output�б����һ�����״̬����һ��ֵ�Ƿ���ڣ�����������ж��Լ��Ƿ�Ͷ��Ʊ������ͶƱ������������ڼ��Զ˵�intput�б��Ƿ���Ҫ�����µ�ͶƱ���
	}

	void ChainObj::VoteInPut(){
		//1.��ȡ�Զ˵�output�б�

		//2.��ȡ�Լ���input�б�

		//3.����Լ���input�б�״̬�����һ�����״̬����һ��ֵ�Ƿ���ڣ���������ж��Լ��Ƿ�Ͷ��Ʊ������ͶƱ������������ڼ��Զ˵�output�б��Ƿ���Ҫ�����µ�ͶƱ���
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
		const PairChainConfigure &pair_chain_a = itr->second;
		a_chain_obj_.SetChainInfo(pair_chain_a.comm_unique_, pair_chain_a.target_comm_unique_);
		a_chain_obj_.SetPeerChain(&b_chain_obj_);

		itr++;
		const PairChainConfigure &pair_chain_b = itr->second;
		b_chain_obj_.SetChainInfo(pair_chain_b.comm_unique_, pair_chain_b.target_comm_unique_);
		b_chain_obj_.SetPeerChain(&a_chain_obj_);

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
		a_chain_obj_.OnTimer(current_time);
		b_chain_obj_.OnTimer(current_time);
	}

	void NotaryMgr::HandleMessage(const std::string &comm_unique, const protocol::WsMessage &message){
		//1���ж��Ƿ������ chain_obj��
		ChainObj * chain = nullptr;
		if (comm_unique == a_chain_obj_.GetChainUnique()){
			chain = &a_chain_obj_;
		}
		else if (comm_unique == b_chain_obj_.GetChainUnique()){
			chain = &b_chain_obj_;
		}

		if (chain == nullptr){
			LOG_ERROR("Can not find chain.");
			return;
		}

		//2������Ϣ�����ڶ�Ӧ��chain_obj ����
		chain->OnHandleMessage(message);
	}
}

