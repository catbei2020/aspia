//
// Aspia Project
// Copyright (C) 2020 Dmitry Chapyshev <dmitry@aspia.ru>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//

#include "peer/authenticator.h"

#include "base/location.h"
#include "base/logging.h"
#include "base/crypto/message_decryptor_openssl.h"
#include "base/crypto/message_encryptor_openssl.h"

namespace peer {

namespace {

constexpr std::chrono::minutes kTimeout{ 1 };

} // namespace

Authenticator::Authenticator(std::shared_ptr<base::TaskRunner> task_runner)
    : timer_(std::move(task_runner))
{
    // Nothing
}

void Authenticator::start(std::unique_ptr<base::NetworkChannel> channel, Callback callback)
{
    if (state() != State::STOPPED)
    {
        LOG(LS_ERROR) << "Trying to start an already running authenticator";
        return;
    }

    channel_ = std::move(channel);
    callback_ = std::move(callback);

    DCHECK(channel_);
    DCHECK(callback_);

    state_ = State::PENDING;

    LOG(LS_INFO) << "Authentication started for: " << channel_->peerAddress();

    // If authentication does not complete within the specified time interval, an error will be
    // raised.
    timer_.start(kTimeout, std::bind(
        &Authenticator::finish, this, FROM_HERE, ErrorCode::UNKNOWN_ERROR));

    channel_->setListener(this);
    if (onStarted())
        channel_->resume();
}

std::unique_ptr<base::NetworkChannel> Authenticator::takeChannel()
{
    if (state() != State::SUCCESS)
        return nullptr;

    return std::move(channel_);
}

// static
const char* Authenticator::osTypeToString(proto::OsType os_type)
{
    switch (os_type)
    {
        case proto::OS_TYPE_WINDOWS:
            return "Windows";

        case proto::OS_TYPE_LINUX:
            return "Linux";

        case proto::OS_TYPE_ANDROID:
            return "Android";

        case proto::OS_TYPE_OSX:
            return "OSX";

        case proto::OS_TYPE_IOS:
            return "IOS";

        default:
            return "Unknown";
    }
}

// static
const char* Authenticator::stateToString(State state)
{
    switch (state)
    {
        case State::STOPPED:
            return "STOPPED";

        case State::PENDING:
            return "PENDING";

        case State::SUCCESS:
            return "SUCCESS";

        case State::FAILED:
            return "FAILED";

        default:
            return "UNKNOWN";
    }
}

// static
const char* Authenticator::errorToString(Authenticator::ErrorCode error_code)
{
    switch (error_code)
    {
        case Authenticator::ErrorCode::SUCCESS:
            return "SUCCESS";

        case Authenticator::ErrorCode::NETWORK_ERROR:
            return "NETWORK_ERROR";

        case Authenticator::ErrorCode::PROTOCOL_ERROR:
            return "PROTOCOL_ERROR";

        case Authenticator::ErrorCode::ACCESS_DENIED:
            return "ACCESS_DENIED";

        case Authenticator::ErrorCode::SESSION_DENIED:
            return "SESSION_DENIED";

        default:
            return "UNKNOWN";
    }
}

void Authenticator::sendMessage(const google::protobuf::MessageLite& message)
{
    DCHECK(channel_);
    channel_->send(base::serialize(message));
}

void Authenticator::finish(const base::Location& location, ErrorCode error_code)
{
    // If the network channel is already destroyed, then exit (we have a repeated notification).
    if (!channel_)
        return;

    channel_->pause();
    channel_->setListener(nullptr);
    timer_.stop();

    if (error_code == ErrorCode::SUCCESS)
        state_ = State::SUCCESS;
    else
        state_ = State::FAILED;

    LOG(LS_INFO) << "Authenticator finished with code: " << errorToString(error_code)
                 << " (" << location.toString() << ")";
    callback_(error_code);
}

void Authenticator::setPeerVersion(const proto::Version& version)
{
    peer_version_ = base::Version(version.major(), version.minor(), version.patch());
}

void Authenticator::onConnected()
{
    // The authenticator receives the channel always in an already connected state.
    NOTREACHED();
}

void Authenticator::onDisconnected(base::NetworkChannel::ErrorCode error_code)
{
    LOG(LS_INFO) << "Network error: " << base::NetworkChannel::errorToString(error_code);

    ErrorCode result = ErrorCode::NETWORK_ERROR;

    if (error_code == base::NetworkChannel::ErrorCode::ACCESS_DENIED)
        result = ErrorCode::ACCESS_DENIED;

    finish(FROM_HERE, result);
}

void Authenticator::onMessageReceived(const base::ByteArray& buffer)
{
    if (state() != State::PENDING)
        return;

    onReceived(buffer);
}

void Authenticator::onMessageWritten(size_t /* pending */)
{
    if (state() != State::PENDING)
        return;

    onWritten();
}

bool Authenticator::onSessionKeyChanged()
{
    LOG(LS_INFO) << "Session key changed";

    std::unique_ptr<base::MessageEncryptor> encryptor;
    std::unique_ptr<base::MessageDecryptor> decryptor;

    if (encryption_ == proto::ENCRYPTION_AES256_GCM)
    {
        encryptor = base::MessageEncryptorOpenssl::createForAes256Gcm(
            session_key_, encrypt_iv_);
        decryptor = base::MessageDecryptorOpenssl::createForAes256Gcm(
            session_key_, decrypt_iv_);
    }
    else
    {
        DCHECK_EQ(encryption_, proto::ENCRYPTION_CHACHA20_POLY1305);

        encryptor = base::MessageEncryptorOpenssl::createForChaCha20Poly1305(
            session_key_, encrypt_iv_);
        decryptor = base::MessageDecryptorOpenssl::createForChaCha20Poly1305(
            session_key_, decrypt_iv_);
    }

    if (!encryptor || !decryptor)
        return false;

    channel_->setEncryptor(std::move(encryptor));
    channel_->setDecryptor(std::move(decryptor));
    return true;
}

} // namespace peer
