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
            , session_(NULL)
        {
            thread_ = new DispatchThread;
        }

        SessionManager::~SessionManager()
        {
            boost::system::error_code ec;
            kill(ec);
            delete thread_;
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

            cancel_timer();

            if (current_ == NULL) {
                current_ = next_ = create_group_with_session(sid, url, resp);
                if (current_) {
                    next_request();
                }
            } else if (next_ != current_) {
                if (!next_->accept(url)) {
                    SessionGroup * group = create_group_with_session(sid, url, resp);
                    if (group) {
                        delete_group(next_);
                        next_ = group;
                    }
                } else {
                    Session * s = new Session(io_svc_, url, resp);
                    sid = s->id();
                    next_->queue_session(s);
                }
            } else if (!next_->accept(url)) {
                SessionGroup * group = create_group_with_session(sid, url, resp);
                if (group) {
                    if (current_->busy()) {
                        next_ = group;
                    } else {
                        delete_group(current_);
                        current_ = next_ = group;
                        next_request();
                    }
                }
            } else {
                Session * s = new Session(io_svc_, url, resp);
                sid = s->id();
                bool need_next = current_->queue_session(s);
                if (need_next) {
                    if (current_->busy()) {
                        boost::system::error_code ec;
                        current_->dispatcher().cancel(ec);
                    } else {
                        next_request();
                    }
                }
            }
        }

        bool SessionManager::setup(
            boost::uint32_t sid, 
            boost::uint32_t index, 
            util::stream::Sink & sink, 
            boost::system::error_code & ec)
        {
            LOG_XXX("setup");

            Session * ses = user_session(sid, ec);
            if (ses) {
                //ses->sink_group().setup(index, sink);
                current_->dispatcher().setup(index, sink, ec);
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
            Session * ses = user_session(sid, ec);
            if (ses) {
                ses->queue_request(Request(range, seek_resp, resp));
                bool need_next = current_->active_session(ses);
                if (need_next) {
                    if (current_->busy()) {
                        current_->dispatcher().cancel(ec);
                    } else {
                        next_request();
                    }
                }
            } else {
                io_svc_.post(boost::bind(seek_resp, ec));
                io_svc_.post(boost::bind(resp, ec));
            }
        }

        bool SessionManager::cancel(
            boost::uint32_t sid,        // 会话ID
            boost::system::error_code & ec)
        {
            LOG_XXX("cancel");

            Session * ses = user_session(sid, ec);
            if (ses && ses == current_->current()) {
                current_->dispatcher().cancel(ec);
            }
            return !ec;
        }

        bool SessionManager::pause(
            boost::uint32_t sid, 
            boost::system::error_code & ec)
        {
            LOG_XXX("pause");

            Session * ses = user_session(sid, ec);
            if (ses && ses == current_->current()) {
                current_->dispatcher().pause(ec);
            }
            return !ec;
        }

        bool SessionManager::resume(
            boost::uint32_t sid, 
            boost::system::error_code & ec)
        {
            LOG_XXX("resume");

            Session * ses = user_session(sid, ec);
            if (ses && ses == current_->current()) {
                current_->dispatcher().resume(ec);
            }
            return !ec;
        }

        bool SessionManager::get_media_info(
            boost::uint32_t sid, 
            ppbox::data::MediaInfo & info, 
            boost::system::error_code & ec)
        {
            LOG_XXX("get_media_info");

            Session * ses = user_session(sid, ec);
            if (ses) {
                return current_->dispatcher().get_media_info(info, ec);
            }
            return false;
        }

        bool SessionManager::get_play_info(
            boost::uint32_t sid, 
            ppbox::data::PlayInfo & info, 
            boost::system::error_code & ec)
        {
            LOG_XXX("get_play_info");

            Session * ses = user_session(sid, ec);
            if (ses) {
                return current_->dispatcher().get_play_info(info, ec);
            }
            return false;
        }

        bool SessionManager::close(
            boost::uint32_t sid, 
            boost::system::error_code & ec)
        {
            LOG_XXX("close");

            Session * ses = current_ ? current_->find_session(sid) : NULL;
            if (ses != NULL) {
                bool need_cancel = current_->close_session(ses);
                if (need_cancel) {
                    current_->dispatcher().cancel(ec);
                }
                if (current_->empty()) {
                    start_timer();
                }
            } else if (next_ != current_) {
                ses = next_->find_session(sid);
                if (ses) {
                    next_->close_session(ses);
                } else {
                    ec = error::session_not_found;
                }
            } else {
                ec = error::session_not_found;
            }
            return ses != NULL;;
        }

        SessionGroup * SessionManager::create_group_with_session(
            boost::uint32_t&  sid, 
            framework::string::Url const & url, 
            response_t const & resp)
        {
            TaskDispatcher * dispatcher = 
                TaskDispatcher::create(io_svc_, thread_->io_svc(), url);
            if (dispatcher) {
                SessionGroup * group = new SessionGroup(url, *dispatcher);
                Session * s = new Session(io_svc_, url, resp);
                sid = s->id();
                group->queue_session(s);
                return group;
            } else {
                io_svc_.post(boost::bind(resp, error::not_support));
                return NULL;
            }
        }

        void SessionManager::delete_group(
            SessionGroup * group)
        {
            boost::system::error_code ec;
            group->dispatcher().close(ec);
            delete &group->dispatcher();
            group->close(error::session_kick_out);
            delete group;
        }

        Session * SessionManager::user_session(
            boost::uint32_t sid,        // 会话ID
            boost::system::error_code & ec)
        {
            Session * ses = current_ ? current_->find_session(sid) : NULL;
            if (ses == NULL) {
                if (next_ != current_ && next_->find_session(sid)) {
                    ec = error::session_not_open;
                } else {
                    ec = error::session_not_found;
                }
            } else if (next_ != current_ || ses != current_->next()) {
                ses = NULL;
                ec = error::session_kick_out;
            } else if (!ses->opened()) {
                ec = error::session_not_open;
                ses = NULL;
            }
            return ses;
        }

        void SessionManager::handle_request(
            boost::system::error_code const & ec)
        {
            boost::uint32_t sid = session_ ? session_->id() : 0;
            LOG_XXX2("handle_request", ec);

            assert(current_);
            // 先处理好内部状态，再调用回调
            SessionGroup * current = current_;
            if (next_ != current_) {
                current_ = next_;
            }
            // 调用回调
            current->response(ec);
            if (current != current_) {
                current->close(error::session_kick_out);
                boost::system::error_code ec1;
                current->dispatcher().close(ec1);
                delete &current->dispatcher();
                delete current;
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
            boost::uint32_t sid = session_ ? session_->id() : 0;
            LOG_XXX("next_request");

            assert(current_);
            assert(current_ == next_);

            Request * req = current_->request();
            if (req == NULL) {
                return;
            }
            if (req == SessionGroup::open_request) {
                session_ = current_->first();
                current_->dispatcher().async_open(current_->url(), 
                    boost::bind(&SessionManager::handle_request, this, _1));
            } else if (req == SessionGroup::switch_request) {
                boost::system::error_code ec;
                session_ = current_->current();
                current_->dispatcher().assign(session_->url(), ec);
                handle_request(ec);
            } else if (req == SessionGroup::buffer_request) {
                current_->dispatcher().async_buffer(
                    boost::bind(&SessionManager::handle_request, this, _1));
            } else if (req == SessionGroup::delete_request) {
                session_ = NULL;
                delete_group(current_);
                current_ = next_ = NULL;
            } else {
                //if (req->session != session_) {
                //    session_ = req->session;
                //    boost::system::error_code ec;
                //    if (!current_->dispatcher().assign(session_->url(), ec) || 
                //        !current_->dispatcher().setup(session_->sink_group(), ec)) {
                //            io_svc().post(
                //                boost::bind(&SessionManager::handle_request, this, ec));
                //            return;
                //    }
                //}
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
            if (current_ && current_->empty()) {
                boost::system::error_code ec1;
                if (current_->busy()) {
                    current_->dispatcher().cancel(ec1);
                } else {
                    delete_group(current_);
                    current_ = next_ = NULL;
                }
            }
        }

    } // namespace dispatch
} // namespace ppbox
