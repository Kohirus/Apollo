#include "rpccontrollerimpl.h"
using namespace apollo;

RpcControllerImpl::RpcControllerImpl()
    : failed_(false)
    , errText_("") {
}

RpcControllerImpl::~RpcControllerImpl() {
}

void RpcControllerImpl::Reset() {
    failed_  = false;
    errText_ = "";
}

bool RpcControllerImpl::Failed() const {
    return failed_;
}

std::string RpcControllerImpl::ErrorText() const {
    return errText_;
}

void RpcControllerImpl::SetFailed(const std::string& reason) {
    failed_  = true;
    errText_ = reason;
}

void RpcControllerImpl::StartCancel() {
}

bool RpcControllerImpl::IsCanceled() const {
    return false;
}

void RpcControllerImpl::NotifyOnCancel(google::protobuf::Closure* callbak) {
}