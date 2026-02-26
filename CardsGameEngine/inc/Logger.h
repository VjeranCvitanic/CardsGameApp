#ifndef _Logger_
#define _Logger_

#include <iostream>
#include <ostream>
#include <fstream>

#include <iomanip>

#include <chrono>
#include <ctime>
#include "time.h"

#include <filesystem>
#include <memory>

#include <string>

#include "Types.h"
#include "EventSink.h"

inline std::ostream& operator<<(std::ostream& os,
                                const fullPlayerId& id)
{
    os << id.first << ", " << id.second;
    return os;
}

#define MAX_NUM_LOGS_IN_ROTATION 9

#define DEBUG 1
#define INFO 2
#define WARNING 3
#define ERROR 4

#define __SHORT_FILE__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define __LOGGER_LOG__(lvl, ...) Logger::Log(lvl, __SHORT_FILE__, __func__ __VA_OPT__(,) __VA_ARGS__) 

#define LOG_DEBUG(...) __LOGGER_LOG__(DEBUG, __VA_ARGS__)
#define LOG_INFO(...)  __LOGGER_LOG__(INFO, __VA_ARGS__)
#define LOG_WARNING(...)  __LOGGER_LOG__(WARNING, __VA_ARGS__)
#define LOG_ERROR(...) __LOGGER_LOG__(ERROR, __VA_ARGS__)

#define DRAW(...) Logger::Draw(__VA_ARGS__);

#define LOG_EVENT(...) Logger::EventLog(__VA_ARGS__);

#define LOG_VAR(...) LOG_DEBUG("Variables: ", __VA_ARGS__)//__, #x "=", (x)) //#__VA_ARGS__

class Logger : public EventSink
{
public:
    Logger(const Logger&) = delete;
    ~Logger();

    static Logger& GetInstance() {
        static Logger instance;
        return instance;
    }

    template<typename... T>
    static void Log(int level_, const char* module, const char* func, const T&... msg)
    {
        if(!Logger::is_on)
            return;
        if(level_ >= level)
        {
            doLog(logOut, level_, module, func, msg...);
        }
    }

    template<typename... T>
    static void doLog(std::ostream* out, int level_, const char* module, const char* func, const T&... msg)
    {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
        #ifdef _WIN32
        // Windows
        localtime_s(&tm, &t);
    #else
        // macOS / Linux
        localtime_r(&t, &tm);
    #endif

        (*out) 
            << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] "
            << "[" << level_to_string(level_) << "] "
            << module << "::"
            << func;
            ((*out << ": " << msg << ' '), ...);
            *out  << std::endl;
    }

    static void Log(int level_, const char* module, const char* func)
    {
        Log(level_, module, func, "");
    }

    template<typename... T>
    static void Draw(const T&... msg)
    {
        ((*drawOut << msg << ' '), ...) << std::flush;
    }

    template<typename... T>
    static void EventLog(const T&... msg)
    {
        doLog(eventOut, 0, "", "", msg...);
    }

    static void logger_setup(const char* logs_dir, const char* draw_dir, const char* event_dir, int level, bool is_on);
    static void setLevel(int lvl);
    static void setLogOutput(const char* path);
    static void setDrawOutput(const char* path);
    static void setEventOutput(const char* file_dir);
    static void setOnOffState(bool is_on);
    static void start();
    static void end();
    void onEvent(const GameEvent& event);
    void LogEvent(const GameEvent& event);
    void LogEv(const PlayerPlayedMoveEvent& e);
    void LogEv(const PlayerDealtCardsEvent& e);
    void LogEv(const StartRoundEvent& e);
    void LogEv(const TressetteDealtCardsEvent& e);
    void LogEv(const StartGameEvent& e);
    void LogEv(const StartBriscolaGameEvent& e);
    void LogEv(const StartMatchEvent& e);
    void LogEv(const RoundOverEvent& e);
    void LogEv(const GameOverEvent& e);
    void LogEv(const MatchOverEvent& e);
    void LogEv(const YourTurnEvent& e);
    void LogEv(const MoveResponseEvent& e);
    void LogEv(const AcussoEvent& e);
    void LogEv(const BriscolaLastRoundEvent& e);

private:
    static std::unique_ptr<Logger> instance;
    static bool is_started;
    static bool is_on;
    static int level;
    static std::ostream* logOut;
    static std::unique_ptr<std::ofstream> logFileStream;
    static std::ostream* drawOut;
    static std::unique_ptr<std::ofstream> drawFileStream;
    static std::ostream* eventOut;
    static std::unique_ptr<std::ofstream> eventFileStream;
    static std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
    static std::chrono::time_point<std::chrono::high_resolution_clock> end_time;

    Logger(){};

    static void setOutput(std::ostream*& output, std::unique_ptr<std::ofstream>& stream, const std::string& filepath);
    static int getNextLogFile(const char* logDir);
    static const char* level_to_string(int level);
    template <typename... T>
    static void print(const T&... msg);
    void flush();
};

inline std::ostream& operator<<(std::ostream& os, MoveReturnValue v)
{
    switch (v)
    {
        case MoveReturnValue::Ok:
            return os << "ok";
        case MoveReturnValue::NotYourTurn:
            return os << "NotYourTurn";
        case MoveReturnValue::CardNotInHand:
            return os << "CardNotInDeck";
        case MoveReturnValue::ColorConstraintNotMet:
            return os << "ColorConstraintNotMet";
        case MoveReturnValue::CantCallIfNotFirstOfHand:
            return os << "CantCallIfNotFirstOfHand";
        case MoveReturnValue::Finish:
            return os << "Finish";
        default:
            break;
    }
    return os << "UnknownMoveValidity";
}

#endif