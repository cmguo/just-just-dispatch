// SessionManager.cpp

#include "ppbox/dispatch/Common.h"
#include "ppbox/dispatch/SessionManager.h"
#include "ppbox/dispatch/Session.h"
#include "ppbox/dispatch/SessionGroup.h"
#include "ppbox/dispatch/DispatchThread.h"
#include "ppbox/dispatch/TaskDispatcher.h"
#include "ppbox/dispatch/Error.h"

#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>

#include <boost/bind.hpp>

namespace ppbox
{
    namespace dispatch
    {

        FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("ppbox.dispatch.SessionManager", framework::logger::Debug);

#define LOG_XXX(func) LOG_INFO("[" func "] sid: " << sid << " current:" << current_ << " next:" << next_);

#define LOG_XXX2(func, ec) LOG_INFO("[" func "] sid: " << sid << " current:" << current_ << " next:" << next_ << " ec:" << ec.message());

        SessionManager::SessionManager(
            boost::asio::io_service & io_svc)
            : io_svc_(io_svc)
            , timer_(io_svc_)
            , timer_id_(0)
            , timer_lanched_(false)
            , current_(NULL)
            , next_(NULL)
            , canceling_(false)
        {
        }

        SessionManager::~SessionManager()
        {
            boost::system::error_code ec;
            kill(ec);
        }

        bool SessionManager::kill(
            boost::system::error_code & ec)
        {
            //boost::system::error_code ec1;
            //if (append_mov_ && append_mov_->next_)
            //{
            //    close(append_mov_->next_->session_id_, ec1);
            //}

            return true;
        }

        void SessionManager::async_open(
            boost::uint32_t&  sid, 
            framework::string::Url const & url, 
            response_t const & resp)
        {
            LOG_XXX("async_open");

            Session * ses = NULL;

            Session * main_ses = NULL;
            std::string session = url.param(param_session);
            bool to_close = false;
            if (!session.empty()) {
                if (session.compare(0, 5, "close") == 0) {
                    to_close = true;
                    session = session.substr(5);
                }
                std::map<std::string, Session *>::iterator iter = named_sessions_.find(session);
                if (iter == named_sessions_.end()) {
                    if (to_close) {
                        io_svc_.post(boost::bind(resp, error::session_not_found));
                        return;
                    }
                    // 先通过下面创建主会话，再继续创建从会话
                } else {
                    main_ses = iter->second;
                }
            }

            boost::system::error_code ec;

            if (main_ses) {
                // 主会话已经存在，不需要做什么
                if (next_ == NULL || main_ses != next_->next()) {
                    ec = error::session_kick_out;
                } else {
                }
            } else if (current_ == NULL) {
                current_ = next_ = create_group(url, ec);
            } else if (next_ != current_) {
                if (!next_->accept(url)) {
                    SessionGroup * group = create_group(url, ec);
                    if (group) {
                        delete_group(next_);
                        next_ = group;
                    }
                } else {
                }
            } else if (!next_->accept(url)) {
                SessionGroup * group = create_group(url, ec);
                if (group) {
                    if (current_->busy()) {
                        if (!canceling_) {
                            canceling_ = true;
                            boost::system::error_code ec;
                            current_->dispatcher().cancel(ec);
                        }
                        next_ = group;
                    } else {
                        delete_group(current_);
                        current_ = next_ = group;
                    }
                }
            } else {
            }

            if (!ec) {
                if (main_ses == NULL) {
                    if (session.empty()) {
                        main_ses = ses = new Session(io_svc_, url, resp);
                    } else {
                        main_ses = new Session(io_svc_, url);
                        ses = new Session(io_svc_, url, resp);
                        named_sessions_[session] = main_ses;
                        main_ses->queue_sub(ses);
                    }

                    bool need_next = next_->queue_session(main_ses);
                    if (current_ != next_ || need_next) {
                        if (current_->busy()) {
                            if (!canceling_) {
                                canceling_ = true;
                                boost::system::error_code ec;
                                current_->dispatcher().cancel(ec);
                            }
                        } else {
                            next_request();
                        }
                    }
                } else {
                    ses = new Session(io_svc_, url, resp);
                    main_ses->queue_sub(ses); // 里面会调用回调
                }
                sid = ses->id();
                cancel_timer();
            } else {
                io_svc_.post(boost::bind(resp, ec));
            }

            if (to_close) {
                named_sessions_.erase(session);
                boost::system::error_code ec;
                close(main_ses->id(), ec);
            }
        }

        bool SessionManager::setup(
            boost::uint32_t sid, 
            boost::uint32_t index, 
            util::stream::Sink & sink, 
            boost::system::error_code & ec)
        {
            LOG_XXX("setup");

            Session * main_ses = NULL;
            Session * ses = user_session(sid, main_ses, ec);
            if (ses) {
                if (ses == main_ses) {
                    current_->dispatcher().setup(index, sink, ec);
                } else {
                    ses->sink_group().setup(index, sink);
                }
            }
            return !ec;
        }

        void SessionManager::async_play(
            boost::uint32_t sid, 
            SeekRange const & range, 
            response_t const & seek_resp,
            response_t const & resp)
        {
            LOG_XXX("async_play");

            boost::system::error_code ec;
            Session * main_ses = NULL;
            Session * ses = user_session(sid, main_ses, ec);
            if (ses) {
                main_ses->queue_request(Request(ses, range, seek_resp, resp));
                bool need_next = current_->active_session(main_ses);
                if (need_next) {
                    if (current_->busy()) {
                        if (!canceling_) {
                            canceling_ = true;
                            current_->dispatcher().cancel(ec);
                        }
                    } else {
                        next_request();
                    }
                }
            } else {
                io_svc_.post(boost::bind(seek_resp, ec));
                io_svc_.post(boost::bind(resp, ec));
            }
        }

        bool SessionManager::seek(
            boost::uint32_t sid, 
            SeekRange & range, 
            boost::system::error_code & ec)
        {
            Session * main_ses = NULL;
            Session * ses = user_session(sid, main_ses, ec);
            if (ses && (ses == main_ses || ses == main_ses->current_sub())) {
                current_->dispatcher().seek(range, ec);
            }
            return !ec;
        }

        bool SessionManager::read(
            boost::uint32_t sid, 
            Sample & sample, 
            boost::system::error_code & ec)
        {
            Session * main_ses = NULL;
            Session * ses = user_session(sid, main_ses, ec);
            if (ses && (ses == main_ses || ses == main_ses->current_sub())) {
                current_->dispatcher().read(sample, ec);
            }
            return !ec;
        }

        bool SessionManager::free(
            boost::uint32_t sid, 
            Sample & sample, 
            boost::system::error_code & ec)
        {
            Session * main_ses = NULL;
            Session * ses = user_session(sid, main_ses, ec);
            if (ses && (ses == main_ses || ses == main_ses->current_sub())) {
                current_->dispatcher().free(sample, ec);
            }
            return !ec;
        }

        bool SessionManager::pause(
            boost::uint32_t sid, 
            boost::system::error_code & ec)
        {
            LOG_XXX("pause");

            Session * main_ses = NULL;
            Session * ses = user_session(sid, main_ses, ec);
            if (ses && (ses == main_ses || ses == main_ses->current_sub())) {
                current_->dispatcher().pause(ec);
            }
            return !ec;
        }

        bool SessionManager::resume(
            boost::uint32_t sid, 
            boost::system::error_code & ec)
        {
            LOG_XXX("resume");

            Session * main_ses = NULL;
            Session * ses = user_session(sid, main_ses, ec);
            if (ses && (ses == main_ses || ses == main_ses->current_sub())) {
                current_->dispatcher().resume(ec);
            }
            return !ec;
        }

        bool SessionManager::get_media_info(
            boost::uint32_t sid, 
            MediaInfo & info, 
            boost::system::error_code & ec)
        {
            LOG_XXX("get_media_info");

            Session * main_ses = NULL;
            Session * ses = user_session(sid, main_ses, ec);
            if (ses) {
                return current_->dispatcher().get_media_info(info, ec);
            }
            return false;
        }

        bool SessionManager::get_stream_info(
            boost::uint32_t sid, 
            std::vector<StreamInfo> & streams, 
            boost::system::error_code & ec)
        {
            LOG_XXX("get_stream_info");

            Session * main_ses = NULL;
            Session * ses = user_session(sid, main_ses, ec);
            if (ses) {
                return current_->dispatcher().get_stream_info(streams, ec);
            }
            return false;
        }

        bool SessionManager::get_stream_status(
            boost::uint32_t sid, 
            StreamStatus & status, 
            boost::system::error_code & ec)
        {
            LOG_TRACE("[get_stream_status] sid: " << sid << " current:" << current_ << " next:" << next_);

            Session * main_ses = NULL;
            Session * ses = user_session(sid, main_ses, ec);
            if (ses) {
                return current_->dispatcher().get_stream_status(status, ec);
            }
            return false;
        }

        bool SessionManager::get_data_stat(
            boost::uint32_t sid, 
            DataStat & stat, 
            boost::system::error_code & ec)
        {
            LOG_TRACE("[get_data_stat] sid: " << sid << " current:" << current_ << " next:" << next_);

            Session * main_ses = NULL;
            Session * ses = user_session(sid, main_ses, ec);
            if (ses) {
                return current_->dispatcher().get_data_stat(stat, ec);
            }
            return false;
        }

        bool SessionManager::cancel(
            boost::uint32_t sid,        // 会话ID
            boost::system::error_code & ec)
        {
            LOG_XXX("cancel");

            Session * main_ses = NULL;
            Session * ses = user_session(sid, main_ses, ec);
            if (ses && (ses == main_ses || ses == main_ses->current_sub())) {
                current_->dispatcher().cancel(ec);
            }
            return !ec;
        }

        bool SessionManager::close(
            boost::uint32_t sid, 
            boost::system::error_code & ec)
        {
            LOG_XXX("close");

            SessionGroup * group = NULL;
            Session * main_session = NULL;
            Session * ses = find_session(sid, group, main_session);
            if (ses == NULL) {
                return false;
            }
            if (ses != main_session) {
                if (main_session->close_sub(ses)) {
                    assert(group == current_);
                    if (!canceling_) {
                        canceling_ = true;
                        current_->dispatcher().cancel(ec);
                    }
                }
            } else if (group == current_) {
                bool need_cancel = current_->close_session(ses);
                if (need_cancel) {
                    if (!canceling_) {
                        canceling_ = true;
                        current_->dispatcher().cancel(ec);
                    }
                }
                if (current_->empty()) {
                    start_timer();
                }
            } else if (group == next_) {
                next_->close_session(ses);
            } else {
                std::vector<Session *>::iterator iter = 
                    std::find(kick_outs_.begin(), kick_outs_.end(), ses);
                assert(iter != kick_outs_.end());
                kick_outs_.erase(iter);
                ses->close(kick_outs_);
                delete ses;
            }
            return true;
        }

        SessionGroup * SessionManager::create_group(
            framework::string::Url const & url, 
            boost::system::error_code & ec)
        {
            TaskDispatcher * dispatcher = 
                TaskDispatcherFactory::create(io_svc_, url);
            if (dispatcher) {
                SessionGroup * group = new SessionGroup(url, *dispatcher);
                return group;
            } else {
                ec = error::not_support;
                return NULL;
            }
        }

        void SessionManager::delete_group(
            SessionGroup * group)
        {
            boost::system::error_code ec;
            group->dispatcher().close(ec);
            delete &group->dispatcher();
            group->close(error::session_kick_out, kick_outs_);
            delete group;
        }

        Session * SessionManager::user_session(
            boost::uint32_t sid,        // 会话ID
            Session *& main_ses, 
            boost::system::error_code & ec)
        {
            SessionGroup * group = NULL;
            Session * ses = find_session(sid, group, main_ses);
            if (ses == NULL) {
                ec = error::session_not_found;
            } else if (group != next_ || main_ses != next_->next()) {
                ses = NULL;
                ec = error::session_kick_out;
            } else if (!ses->opened()) {
                ses = NULL;
                ec = error::session_not_open;
            }
            return ses;
        }

        Session * SessionManager::find_session(
            boost::uint32_t sid,        // 会话ID
            SessionGroup *& group, 
            Session *& main_ses)
        {
            Session * ses = current_ ? current_->find_session(sid, main_ses) : NULL;
            if (ses) {
                group = current_;
            } else {
                if (next_ != current_ && (ses = next_->find_session(sid, main_ses))) {
                    group = next_;
                } else {
                    for (size_t i = 0; i < kick_outs_.size(); ++i) {
                        if ((ses = (kick_outs_[i]->id() == sid ? kick_outs_[i] : NULL)) || (ses = kick_outs_[i]->find_sub(sid))) {
                            main_ses = kick_outs_[i];
                            group = (SessionGroup *)1; // deleted group
                            return ses;
                        }
                    }
                }
            }
            return ses;
        }

        static inline boost::uint32_t current_id(
            SessionGroup * group)
        {
            Session * ses = group->current();
            if (ses == NULL || ses == SessionGroup::delete_session)
                return 0;
            if (ses->current_sub()) {
                return ses->current_sub()->id();
            } else {
                return ses->id();
            }
        }

        void SessionManager::handle_request(
            boost::system::error_code const & ec)
        {
            boost::uint32_t sid = current_id(current_);
            LOG_XXX2("handle_request", ec);

            canceling_ = false;

            assert(current_);
            // 先处理好内部状态，再调用回调
            SessionGroup * current = current_;
            if (next_ != current_) {
                current_ = next_;
            }
            // 调用回调
            current->response(ec);
            if (current != current_) {
                delete_group(current);
            }

            if (timer_lanched_) {
                boost::system::error_code ec;
                handle_timer(timer_id_, ec);
            }

            if (current_) { // handle_timer有可能删除了current_
                if (current_->empty()) { // 在应答中，可能删除了延迟删除的会话
                    start_timer();
                }
                next_request();
            }
        }

        void SessionManager::next_request()
        {
            boost::uint32_t sid = current_id(current_);
            LOG_XXX("next_request");

            assert(current_);
            assert(current_ == next_);

            Request * req = current_->request();
            if (req == NULL) {
                return;
            }
            if (req == SessionGroup::open_request) {
                current_->dispatcher().async_open(current_->url(), 
                    boost::bind(&SessionManager::handle_request, this, _1));
            } else if (req == SessionGroup::switch_request) {
                boost::system::error_code ec;
                current_->dispatcher().switch_to(current_->current()->url(), ec);
                handle_request(ec);
            } else if (req == Session::setup_request) {
                boost::system::error_code ec;
                current_->dispatcher().setup(current_->current()->current_sub()->sink_group(), ec);
                handle_request(ec);
            } else if (req == SessionGroup::buffer_request) {
                current_->dispatcher().async_buffer(
                    boost::bind(&SessionManager::handle_request, this, _1));
            } else if (req == SessionGroup::delete_request) {
                delete_group(current_);
                current_ = next_ = NULL;
            } else {
                current_->dispatcher().async_play(req->range, req->seek_resp, 
                    boost::bind(&SessionManager::handle_request, this, _1));
            }
        }

        void SessionManager::start_timer()
        {
            ++timer_id_;
            LOG_INFO("[start_timer] timer_id:" << timer_id_);
            timer_.expires_from_now(
                framework::timer::Duration::seconds(5));
            timer_lanched_ = false;
            timer_.async_wait(
                boost::bind(&SessionManager::handle_timer, this, timer_id_, _1));
        }

        void SessionManager::cancel_timer()
        {
            LOG_INFO("[start_timer] timer_id:" << timer_id_);
            ++timer_id_;
            timer_lanched_ = false;
            timer_.cancel();
        }

        void SessionManager::handle_timer(
            boost::uint32_t timer_id, 
            boost::system::error_code const & ec)
        {
            LOG_INFO("[handle_timer] timer_id:" << timer_id << "/" << timer_id_);

            if (timer_id != timer_id_)
                return;
            timer_lanched_ = true;
            if (current_ && current_ == next_ && current_->empty()) {
                if (current_->busy()) {
                    if (!canceling_) {
                        canceling_ = true;
                        boost::system::error_code ec1;
                        current_->dispatcher().cancel(ec1);
                    }
                } else {
                    delete_group(current_);
                    current_ = next_ = NULL;
                }
            }
        }

    } // namespace dispatch
} // namespace ppbox
