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
using namespace framework::logger;

#include <boost/bind.hpp>

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("ppbox.dispatch.SessionManager", framework::logger::Debug);

namespace ppbox
{
    namespace dispatch
    {

#define LOG_PATHER(p) LOG_INFO("["<<p<<"] cur:"<<current_<<" append:"<<next_);

        SessionManager::SessionManager(
            boost::asio::io_service & io_svc)
            : io_svc_(io_svc)
            , timer_(io_svc_)
            , time_id_(0)
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
            LOG_PATHER("kill");
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
            LOG_PATHER("async_open");

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
            } else if (!next_->busy()) {
                boost::system::error_code ec;
                next_->dispatcher().close(ec);
                delete &next_->dispatcher();
                next_->close(error::session_kick_out);
                delete next_;
                if (current_ == next_) {
                    current_ = next_ = m;
                } else {
                    next_ = m;
                }
            } else {
                assert(current_ == next_);
                boost::system::error_code ec;
                current_->dispatcher().cancel(ec);
            }

            return;
        }

        bool SessionManager::setup(
            boost::uint32_t sid, 
            boost::uint32_t index, 
            util::stream::Sink & sink, 
            boost::system::error_code & ec)
        {
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
            boost::system::error_code ec;
            Session * ses = user_session(sid, ec);
            if (ses) {
                ses->queue_request(Request(range, seek_resp, resp));
                current_->active_session(ses);
            } else {
                io_svc_.post(boost::bind(seek_resp, ec));
                io_svc_.post(boost::bind(resp, ec));
            }
        }

        bool SessionManager::cancel(
            boost::uint32_t sid,        // 会话ID
            boost::system::error_code & ec)
        {
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
            
            Session * ses = current_ ? current_->find_session(sid) : NULL;
            if (ses != NULL) {
                current_->close_session(ses);
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
            assert(current_);
            current_->response(ec);
            if (next_ != current_) {
                current_->close(error::session_kick_out);
                current_ = next_;
            }
            Request * req = current_->request();
            if (req == NULL)
                return;
            if (req == SessionGroup::open_request) {
                current_->dispatcher().async_open(current_->url(), 
                    boost::bind(&SessionManager::handle_request, this, _1));
            } else if (req == SessionGroup::buffer_request) {
                current_->dispatcher().async_buffer(
                    boost::bind(&SessionManager::handle_request, this, _1));
            } else {
                if (req->session != session_) {
                    session_ = req->session;
                    boost::system::error_code ec1;
                    if (!current_->dispatcher().setup(session_->sink_group(), ec1)) {
                        io_svc().post(
                            boost::bind(&SessionManager::handle_request, this, ec1));
                        return;
                    }
                }
                current_->dispatcher().async_play(req->range, req->seek_resp, 
                    boost::bind(&SessionManager::handle_request, this, _1));
            }
        }

        void SessionManager::handle_timer(
            boost::system::error_code const & ec)
        {

        }

/*
        void Manager::cancel_session(SessionGroup* move)
        {
            LOG_PATHER("cancel_session");
            boost::system::error_code ec;
            //判断 buffering openning cancel_delay playing next_play
            if (!move->openned_)
            {//openning
                if (move->sessions_.size() > 0)
                {
                    resonse_session(move,SF_NONE);
                }
                else
                {
                    ++time_id_;
                    cancel_wait(ec);
                }
                interface_->cancel_open_playlink(ec);
            }
            else 
            {
                if(move->sessions_.size() < 1)
                {
                    ++time_id_;
                    cancel_wait(ec);
                    interface_->cancel_buffering(ec);
                }
                if(move->current_->playlist_.size() > 0)
                {
                    interface_->cancel_play_playlink(ec);
                }
                
                if (move->next_->playlist_.size() > 0)
                {
                    resonse_player(move->next_);
                }
                resonse_session(move,SF_FRONT_NO_RESP);
            }
        }

        void Manager::open_callback(boost::system::error_code const & ec)
        {
            LOG_PATHER("open_callback ec:"<<ec.message());
            assert(!cur_mov_->openned_);
            if (cur_mov_ == append_mov_)
            {
                open_callback_one(ec);
            }
            else
            {
                open_callback_two(ec);
            }
        }

        void Manager::open_callback_one(boost::system::error_code const & ec)
        {//  openning  cancel_delay 
            if (!ec)
            {
                append_mov_->openned_ = true;
                if (cur_mov_->sessions_.size() > 0)
                {//openning
                    boost::system::error_code ec1;
                    resonse_session(append_mov_,SF_ALL,ec1);
                }
                else
                {//cancel_delay
                    Session* s = new Session(ios_);
                    append_mov_->current_ = append_mov_->next_ = s;
                    async_wait(10000,boost::bind(&Manager::wait_callback,this,time_id_,_1));
                    interface_->async_buffering(s,boost::bind(&Manager::buffering_callback,this,_1));
                }
            } 
            else
            {
                if (cur_mov_->sessions_.size() > 0)
                {//openning
                    resonse_session(cur_mov_,SF_NONE);
                }

                delete cur_mov_;
                cur_mov_ = append_mov_ = NULL;
            }

        }

        void Manager::open_callback_two(boost::system::error_code const & ec)
        {// canceling
            assert(cur_mov_ != append_mov_);
            assert(cur_mov_->sessions_.size() < 1);


            delete cur_mov_;
            cur_mov_ = append_mov_;

            if (append_mov_->sessions_.size() < 1)
            {
                delete append_mov_;
                append_mov_ = cur_mov_ = NULL;
                return;
            }
            get_interface(append_mov_->sessions_[0]->format_);
            interface_->async_open_playlink(append_mov_->play_link_,append_mov_->sessions_[0]->format_,boost::bind(&Manager::open_callback,this,_1));
        }


        void Manager::play_callback(boost::system::error_code const & ec)
        { // playling  next_session  play_canceling
            LOG_PATHER("play_callback ec:"<<ec.message());
            if (cur_mov_ == append_mov_)
            {
                play_callback_one(ec);
            }
            else
            {
                play_callback_two(ec);
            }
        }

        void Manager::play_callback_one(boost::system::error_code const & ec)
        {//playling  next_session
            if (append_mov_->current_ != append_mov_->next_)
            {//next_session
                clear_session(append_mov_,append_mov_->current_);
                append_mov_->current_ = append_mov_->next_;
            }
            else
            {//playing
                //清除第0个play
                resonse_player(append_mov_->current_,PF_FRONT);
            }

            if (append_mov_->current_->playlist_.size() < 1)
            {
                clear_session(append_mov_,append_mov_->current_);
                append_mov_->next_ = append_mov_->current_ = NULL;

                if (append_mov_->sessions_.size() < 1)
                {//转close_delay
                    Session* s = new Session(ios_);
                    append_mov_->next_ = append_mov_->current_ = s;
                    async_wait(10000,boost::bind(&Manager::wait_callback,this,time_id_,_1));
                    interface_->async_buffering(s,boost::bind(&Manager::buffering_callback,this,_1));
                }
                else
                {
                    //转openned
                }
            }
            else
            {
                interface_->async_play_playlink(append_mov_->current_,boost::bind(&Manager::play_callback,this,_1));
            }
        }

        void Manager::play_callback_two(boost::system::error_code const & ec)
        {//play_canceling
            //assert(cur_mov_->sessions_.size() > 0);
            boost::system::error_code ec1;
            interface_->close_playlink(ec1);

            clear_session(cur_mov_,cur_mov_->current_);
            assert(cur_mov_->sessions_.size() < 1);
            delete cur_mov_;
            cur_mov_ = append_mov_;

            if (append_mov_->sessions_.size() < 1)
            {
                delete append_mov_;
                append_mov_ = cur_mov_ = NULL;
                return;
            }

            get_interface(append_mov_->sessions_[0]->format_);
            interface_->async_open_playlink(append_mov_->play_link_,append_mov_->sessions_[0]->format_,boost::bind(&Manager::open_callback,this,_1));
        }

        void Manager::setup(
            boost::uint32_t session_id,
            size_t control,
            util::stream::Sink* sink,
            boost::system::error_code& ec)
        {
            LOG_PATHER("setup");
            SessionGroup* m = NULL;
            Session* s = NULL;
            find_session(session_id,m,s,ec);
            if (!ec)
            {
                if (m == cur_mov_ && s != m->current_)
                {
                     s->sinks_.add(control,sink);
                }
                else
                {
                    ec == error::wrong_status;
                }
            }
        }


        void Manager::buffering_callback(boost::system::error_code const & ec)
        {//buffering失败 新的打开
            LOG_PATHER("buffering_callback ec:"<<ec.message());
            if (append_mov_ == cur_mov_)
            {
                if(append_mov_->current_ != append_mov_->next_)
                {
                    play_callback_one(ec);
                    return;
                }
                else if (append_mov_->sessions_.size() > 0)
                {
                    clear_session(append_mov_,append_mov_->current_);
                    append_mov_->current_ = append_mov_->next_ = NULL;
                    return;
                }
                else
                {
                    boost::system::error_code ec1;
                    ++time_id_;
                    cancel_wait(ec1);
                    append_mov_ = new SessionGroup();
                }
            }
            play_callback_two(ec);
        }

        void Manager::wait_callback(const boost::uint32_t time_id,boost::system::error_code const & ec)
        {
            LOG_PATHER("wait_callback ec:"<<ec.message());
            if (time_id != time_id_)
            {
                return;
            }

            ++time_id_;
            
            boost::system::error_code ec1;
            
            //cancel_delay buffering
            assert(cur_mov_ == append_mov_);

            append_mov_ = new SessionGroup();

            if (cur_mov_->openned_)
            {//buffering
                interface_->cancel_buffering(ec1);
            }
            else
            {//cancel_delay
                interface_->cancel_open_playlink(ec1);
            }
        }

        boost::system::error_code Manager::async_play(
            SessionGroup* move 
            ,Session* s
            , boost::uint32_t beg
            , boost::uint32_t end
            ,ppbox::dispatch::session_callback_respone const &resp)
        {
            boost::system::error_code ec;
            if (move->openned_)
            {
                PlayRequest player(beg,end,resp);

                if (move->next_ == NULL)
                {
                    move->current_ = move->next_ = s;
                    s->playlist_.push_back(player);
                    interface_->async_play_playlink(s,boost::bind(&Manager::play_callback,this,_1));
                    return ec;
                } 
                else
                {
                    if (move->next_ == s )
                    {
                        if (s->playlist_.size() > 0)
                        {
                            resonse_player(s);
                        }
                    }
                    else if (move->current_ == s)
                    {
                        ec = error::wrong_status;
                    }
                    else
                    {
                        resonse_player(move->next_);
                        if (move->next_ != move->current_)
                        {
                            clear_session(move,move->next_);
                        }
                        else
                        {//关闭前一个播放，准备播下一个播放
                            interface_->cancel_play_playlink(ec);
                        }
                        move->next_ = s;
                    }

                }

                if (!ec)
                {
                    s->playlist_.push_back(player);
                }
            }
            else
            {
                ec = error::play_not_open_moive;
            }
            return ec;

        }

        void Manager::async_play(boost::uint32_t session_id
            , boost::uint32_t beg
            , boost::uint32_t end
            ,ppbox::dispatch::session_callback_respone const &resp)
        {
            LOG_PATHER("async_play");
            boost::system::error_code ec;

            SessionGroup* m = NULL;
            Session* s = NULL;
            find_session(session_id,m,s,ec);
            
            if (!ec)
            {
                if (m == cur_mov_)
                {
                    ec = async_play(m,s,beg,end,resp);
                }
                else
                {
                    ec = error::canceled_moive;
                }

            }

            if (ec)
            {
                assert(0);
                resp(ec);
            }
        }

        void Manager::close(boost::uint32_t session_id,boost::system::error_code& ec)
        {
            LOG_PATHER("close");
            SessionGroup* m = NULL;
            Session* s = NULL;
            find_session(session_id,m,s,ec);

            if (!ec)
            {
                if (m == append_mov_)
                {
                    close(m,s,error::close_moive);
                }
                else
                {
                    ec = error::canceled_moive;
                }
            }

        }

        void Manager::close(SessionGroup* m,Session* s,boost::system::error_code const & ec)
        { //openning openned  next_session playling
            if (append_mov_ == cur_mov_)
            {
                if (!m->openned_)
                {
                    response(s);
                }
                else if (m->current_ == s)
                {//播放或切换player时 清session时过早
                    if (s->playlist_.size() > 0)
                    {//playing
                        resonse_player(s);
                        boost::system::error_code ec1;
                        interface_->cancel_play_playlink(ec1);
                    }
                    else
                    {//nextplay

                    }
                    return;
                }
                
                clear_session(m,s);
                
                if (m->next_ == s) //two session
                {
                    Session* sTemp = new Session(ios_);
                    m->next_ = sTemp;
                }
                else 
                {
                    if (m->sessions_.size() < 1)
                    {
                        async_wait(10000,boost::bind(&Manager::wait_callback,this,time_id_,_1));
                        if (m->openned_)
                        {
                            interface_->async_buffering(s,boost::bind(&Manager::buffering_callback,this,_1));
                        }
                    }
                }
            }
            else
            {
                response(s);
                clear_session(m,s);
                if (m->sessions_.size() < 1)
                {
                    Session* sTemp = new Session(ios_);
                    m->current_ = m->next_ = sTemp;
                }
            }
        }


        void Manager::pause(boost::uint32_t session_id,boost::system::error_code& ec)
        {

            LOG_PATHER("pause");
            if(NULL == cur_mov_ 
                || cur_mov_->sessions_.size() < 1 
                || cur_mov_->sessions_[0]->playlist_.size() < 1
                || cur_mov_->sessions_[0]->session_id_ != session_id)
            {
                assert(0);
                ec = boost::asio::error::operation_aborted;
            }
            else
            {
                interface_->pause_moive(ec);
            }
        }

        void Manager::resume(boost::uint32_t session_id,boost::system::error_code& ec)
        {
            LOG_PATHER("resume");
            if(NULL == cur_mov_ 
                || cur_mov_->sessions_.size() < 1 
                || cur_mov_->sessions_[0]->playlist_.size() < 1
                || cur_mov_->sessions_[0]->session_id_ != session_id)
            {
                ec = boost::asio::error::operation_aborted;
            }
            else
            {
                interface_->resume_moive(ec);
            }
        }

        boost::system::error_code Manager::get_media_info(
            ppbox::dispatch::MediaInfo & info)
        {
            return interface_->get_media_info(info);
        }

        boost::system::error_code Manager::get_play_info(
            ppbox::dispatch::PlayInfo & info)
        {
            return interface_->get_play_info(info);
        }

        void Manager::async_wait(
            boost::uint32_t wait_timer
            , ppbox::dispatch::session_callback_respone const &resp)
        {
            LOG_PATHER("async_wait");
            timer_.expires_from_now(boost::posix_time::milliseconds(wait_timer));
            timer_.async_wait(resp);
        }

        void Manager::cancel_wait(boost::system::error_code& ec)
        {
            LOG_PATHER("cancel_wait");
            timer_.cancel();
        }


        void Manager::find_session(
            const boost::uint32_t session_id,
            SessionGroup* &move,
            Session* &s,
            boost::system::error_code & ec)
        {
            if (NULL == append_mov_)
            {
                ec = error::wrong_status;
                return;
            }

            SessionGroup::Iter iter = std::find_if(append_mov_->sessions_.begin(),
                append_mov_->sessions_.end(),FindBySession(session_id));
            if (iter != append_mov_->sessions_.end())
            {
                move = append_mov_;
                s = *iter;
            }
            else if (append_mov_ != cur_mov_ && cur_mov_->sessions_.end() != 
                (iter = std::find_if(cur_mov_->sessions_.begin(),
                cur_mov_->sessions_.end(),FindBySession(session_id)))
                )
            {
                move = cur_mov_;
                s = *iter;
            }
            else
            {
                ec = error::not_find_session;
            }
        }

        void Manager::resonse_player(Session* session,PlayerFlag pf )
        {
            boost::system::error_code ec = boost::asio::error::operation_aborted;
            switch (pf)
            {
            case PF_ALL:
                {
                    for(Session::Iter iter = session->playlist_.begin();
                        iter != session->playlist_.end();
                        ++iter)
                    {
                        ios_.post(boost::bind((*iter).resp_,ec));
                    }
                    session->playlist_.clear();
                }
                break;
            case PF_FRONT:
                {
                    Session::Iter iter = session->playlist_.begin();
                    if(iter != session->playlist_.end())
                    {
                        ios_.post(boost::bind((*iter).resp_,ec));
                        session->playlist_.erase(iter);
                    }
                }
                break;
            }

        }

        void Manager::response(Session* s,boost::system::error_code const & ec)
        {
            ios_.post(boost::bind(s->resp_,ec));
        }

        void Manager::resonse_session(SessionGroup* move,SessionFlag sf,boost::system::error_code const & ec)
        {
            Session* s = NULL;
            
            if (move->sessions_.size() < 1)
            {
                return;
            }

            switch (sf)
            {
            case SF_ALL:
                {
                    for (SessionGroup::Iter iter = move->sessions_.begin();
                        iter != move->sessions_.end();
                        ++iter)
                    {
                        s = (*iter);
                        ios_.post(boost::bind(s->resp_,ec));
                    }
                }
                break;
            case SF_FRONT_NO_RESP:
                {
                    Session* front = move->current_;
                    for (SessionGroup::Iter iter = move->sessions_.begin(); iter != move->sessions_.end();++iter)
                    {
                        if (front != *iter)
                        {
                            s = (*iter);
                            delete s;
                            s = NULL;
                        }
                    }
                    move->sessions_.clear();
                    move->sessions_.push_back(front);
                }
                break;
            case SF_NONE:
                {
                    for (SessionGroup::Iter iter = move->sessions_.begin();
                        iter != move->sessions_.end();
                        ++iter)
                    {
                        s = (*iter);
                        ios_.post(boost::bind(s->resp_,ec));
                        delete s;
                    }
                    move->sessions_.clear();
                }
                break;
            case SF_NONE_NO_RESP:
                {
                    for (SessionGroup::Iter iter = move->sessions_.begin();
                        iter != move->sessions_.end();
                        ++iter)
                    {
                        s = (*iter);
                        delete s;
                    }
                    move->sessions_.clear();
                }
                break;
            default:
                break;
            }
        }
*/
    } // namespace dispatch
} // namespace ppbox

