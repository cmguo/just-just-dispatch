// SessionManager.cpp

#include "ppbox/dispatch/Common.h"
#include "ppbox/dispatch/SessionManager.h"
#include "ppbox/dispatch/Session.h"
#include "ppbox/dispatch/SessionGroup.h"
#include "ppbox/dispatch/DispatchThread.h"
#include "ppbox/dispatch/Dispatcher.h"
#include "ppbox/dispatch/Error.h"

#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>

#include <boost/bind.hpp>

namespace ppbox
{
    namespace dispatch
    {

        FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("ppbox.dispatch.SessionManager", framework::logger::Debug);

#define LOG_XXX(p) LOG_INFO("[" p "] sid: " << sid << " current:" << current_ << " next:" << next_);

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

            SessionGroup * m = next_;
            if (m == NULL || !m->accept(url)) {
                Dispatcher * dispatcher = 
                    Dispatcher::create(io_svc_, thread_->io_svc(), url);
                if (dispatcher) {
                    m = new SessionGroup(url, *dispatcher);
                } else {
                    io_svc_.post(boost::bind(resp, error::not_support));
                    return;
                }
            }

            Session * s = new Session(url, resp);
            sid = s->id();
            m->queue_session(s);

            if (m == next_) {
                return;
            }

            if (current_ == NULL) {
                current_ = next_ = m;
                next_request();
            } else if (!next_->busy()) {
                boost::system::error_code ec;
                next_->dispatcher().close(ec);
                delete &next_->dispatcher();
                next_->close(error::session_kick_out);
                delete next_;
                if (current_ == next_) {
                    current_ = next_ = m;
                    next_request();
                } else {
                    next_ = m;
                }
            } else {
                assert(current_ == next_);
                boost::system::error_code ec;
                current_->dispatcher().cancel(ec);
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
                // setup 不会立即更新到 Dispatcher
                ses->sink_group().setup(index, sink);
            }
            return ses != NULL;;
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
            Session * ses = user_session(sid, ec);
            if (ses) {
                return current_->dispatcher().get_media_info(info, ec);
            }
            return false;
        }

        bool SessionManager::get_play_info(
            boost::uint32_t sid, 
            ppbox::data::MediaInfo & info, 
            boost::system::error_code & ec)
        {
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
                    if (current_->busy()) {
                        current_->dispatcher().cancel(ec);
                    } else {
                        start_timer();
                    }
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
            } else if (next_ != current_) {
                ses = NULL;
                ec = error::status_refuse;
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
            LOG_XXX("handle_request");

            assert(current_);
            current_->response(ec);
            if (next_ != current_) {
                current_->close(error::session_kick_out);
                current_ = next_;
            }

            next_request();
        }

        void SessionManager::next_request()
        {
            assert(current_);
            assert(current_ == next_);
            Request * req = current_->request();
            if (req == NULL) {
                if (timer_lanched_) {
                    boost::system::error_code ec;
                    handle_timer(timer_id_, ec);
                }
                return;
            }
            cancel_timer();
            if (req == SessionGroup::open_request) {
                session_ = NULL;
                current_->dispatcher().async_open(current_->url(), 
                    boost::bind(&SessionManager::handle_request, this, _1));
            } else if (req == SessionGroup::buffer_request) {
                session_ = NULL;
                current_->dispatcher().async_buffer(
                    boost::bind(&SessionManager::handle_request, this, _1));
                start_timer();
            } else if (req == SessionGroup::delete_request) {
                session_ = NULL;
                boost::system::error_code ec;
                current_->dispatcher().close(ec);
                delete &current_->dispatcher();
                delete current_;
                current_ = next_ = NULL;
            } else {
                if (req->session != session_) {
                    session_ = req->session;
                    boost::system::error_code ec;
                    if (!current_->dispatcher().assign(session_->url(), ec) || 
                        !current_->dispatcher().setup(session_->sink_group(), ec)) {
                            io_svc().post(
                                boost::bind(&SessionManager::handle_request, this, ec));
                            return;
                    }
                }
                current_->dispatcher().async_play(req->range, req->seek_resp, 
                    boost::bind(&SessionManager::handle_request, this, _1));
            }
        }

        void SessionManager::start_timer()
        {
            timer_.expires_from_now(
                framework::timer::Duration::seconds(5));
            ++timer_id_;
            timer_lanched_ = false;
            timer_.async_wait(
                boost::bind(&SessionManager::handle_timer, this, timer_id_, _1));
        }

        void SessionManager::cancel_timer()
        {
            ++timer_id_;
            timer_lanched_ = false;
            timer_.cancel();
        }

        void SessionManager::handle_timer(
            boost::uint32_t time_id, 
            boost::system::error_code const & ec)
        {
            if (time_id != timer_id_)
                return;
            timer_lanched_ = true;
            if (current_ && current_->empty()) {
                boost::system::error_code ec1;
                if (current_->busy()) {
                    current_->dispatcher().cancel(ec1);
                } else {
                    current_->dispatcher().close(ec1);
                    delete &current_->dispatcher();
                    delete current_;
                    current_ = next_ = NULL;
                }
            }
        }

    } // namespace dispatch
} // namespace ppbox
