#include "Logger.h"
#include "Cards.h"
#include "Acussos.h"

std::unique_ptr<std::ofstream> Logger::logFileStream;
std::unique_ptr<std::ofstream> Logger::drawFileStream;
std::unique_ptr<std::ofstream> Logger::eventFileStream;

bool Logger::is_on = false;
int Logger::level = INFO;
std::ostream* Logger::logOut(&std::cout);
std::ostream* Logger::drawOut(&std::cout);
std::ostream* Logger::eventOut(&std::cout);

bool Logger::is_started = false;
std::chrono::time_point<std::chrono::high_resolution_clock> Logger::start_time = {};
std::chrono::time_point<std::chrono::high_resolution_clock> Logger::end_time = {};


Logger::~Logger()
{
    Logger::end();
    print("\nLOGGING ENDED\n");
    flush();
    if(logFileStream)
    {
        logFileStream->close();
        logFileStream.reset();
    }
    if(drawFileStream)
    {
        drawFileStream->close();
        drawFileStream.reset();
    }
    if(eventFileStream)
    {
        eventFileStream->close();
        eventFileStream.reset();
    }
}

// Class functions
void Logger::logger_setup(const char* logs_dir, const char* draw_dir, const char* event_dir, int level, bool is_on)
{
    Logger::GetInstance().start();
    Logger::GetInstance().setLogOutput(logs_dir);
    Logger::GetInstance().setDrawOutput(draw_dir);
    Logger::GetInstance().setEventOutput(event_dir);
    Logger::GetInstance().setLevel(level);
    Logger::GetInstance().setOnOffState(is_on);
}

void Logger::setOnOffState(bool is_on_)
{
    is_on = is_on_;
}

void Logger::start()
{
    Logger::is_started = true;
    start_time = std::chrono::high_resolution_clock::now();
}

void Logger::end()
{
    if(is_started)
    {
        end_time = std::chrono::high_resolution_clock::now();
        std::chrono::microseconds dur = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

        print("\nExecution time: ", (float)(dur.count() / 1000.0), " miliseconds");
    }
}

template <typename... T>
void Logger::print(const T&... msg)
{
    (*logOut << ... << msg) << std::endl;
}

void Logger::setLevel(int lvl)
{
    level = lvl;
}

void Logger::setOutput(std::ostream*& output, std::unique_ptr<std::ofstream>& ofstream, const std::string& filepath)
{
    if(filepath == "stdout")
    {
        ofstream.reset();
        output = &std::cout;
    }
    else
    {
        ofstream = std::make_unique<std::ofstream>(filepath, std::ios::trunc);
        if (!ofstream->is_open()) {
            ofstream.reset();
            output = &std::cout;
        }
        else
        {   
            output = ofstream.get();
        }
    }
}

void Logger::setLogOutput(const char* file_dir)
{
    if(!file_dir)
        return;
    int nextLog = 1; //getNextLogFile(file_dir)
    std::string full_filepath = std::string(file_dir) + std::string("log") + std::to_string(nextLog) + ".log";
    setOutput(logOut, logFileStream, full_filepath);
    print("\n\nLOGGING STARTED\n");
    print("Output: ", full_filepath, "\n");
}

void Logger::setDrawOutput(const char* file_dir)
{
    if(!file_dir)
        return;
    int nextLog = 1; //getNextLogFile(file_dir)
    std::string full_filepath = std::string(file_dir) + std::string("drw") + std::to_string(nextLog) + ".txt";
    setOutput(drawOut, drawFileStream, full_filepath);
    Draw("\n\nDRAWING STARTED\n");
}

void Logger::setEventOutput(const char* file_dir)
{
    if(!file_dir)
        return;
    int nextLog = 1; //getNextLogFile(file_dir)
    std::string full_filepath = std::string(file_dir) + std::string("event") + std::to_string(nextLog) + ".txt";
    setOutput(eventOut, eventFileStream, full_filepath);
    EventLog("\n\nEventLog STARTED\n");
}

// TODO - timestamp
int Logger::getNextLogFile(const char* logDir)
{
    if (!std::filesystem::exists(logDir))
    {
        std::filesystem::create_directories(logDir);
    }

    std::vector<int> logNumbers;

    for (const auto& entry : std::filesystem::directory_iterator(logDir)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            if ((filename.find("log") == 0 && filename.find(".log") != std::string::npos) || (filename.find("drw") == 0 && filename.find(".txt") != std::string::npos)) {
                try {
                    int num = std::stoi(filename.substr(3, filename.size() - 7)); // logX.log -> X
                    logNumbers.push_back(num);
                } catch (...) {
                    // ignore non-conforming files
                }
            }
        }
    }

    int nextLog = 0;
    if (!logNumbers.empty()) {
        nextLog = (*std::max_element(logNumbers.begin(), logNumbers.end())) % (MAX_NUM_LOGS_IN_ROTATION) + 1;
    }

    return nextLog;
}

const char* Logger::level_to_string(int level)
{
    switch(level)
    {
        case DEBUG:
            return "DEBUG";
        case INFO:
            return "INFO";
        case WARNING:
            return "WARNING";
        case ERROR:
            return "ERROR";
        default:
            return "";
    }
}

void Logger::flush() {
    if (logOut)
        logOut->flush();
    if (drawOut)
        drawOut->flush();
    if(eventOut)
        eventOut->flush();
}

void Logger::onEvent(const GameEvent& event)
{
	LOG_EVENT("=================================== Event received ===================================");
    LogEvent(event);
}

void Logger::LogEvent(const GameEvent& event)
{
    std::visit([this](const auto& e)
    {
        this->LogEv(e);
    }, event);
}

void Logger::LogEv(const PlayerPlayedMoveEvent& e)
{
    LOG_EVENT("Player ", e.move.playerId, " played card ", Cards::CardToString(e.move.card));
}

void Logger::LogEv(const PlayerDealtCardsEvent& e)
{
    LOG_EVENT("Player ", e.playerId, " was dealt ", e.cards.size(), " cards");
    for(auto& card : e.cards)
        LOG_EVENT(Cards::CardToString(card));
}

void Logger::LogEv(const StartRoundEvent& e)
{
    LOG_EVENT("Start Round, first to play: ", e.firstToPlayId);
}

void Logger::LogEv(const TressetteDealtCardsEvent& e)
{
    LOG_EVENT("PlayerId: ", e.playerId.second, ", dealtCards: ");
    for(auto& card : e.dealtCards)
        LOG_EVENT(Cards::CardToString(card));
}

void Logger::LogEv(const StartGameEvent& e)
{
    LOG_EVENT("Start Game, first to play: ", e.firstToPlayId);
}

void Logger::LogEv(const StartBriscolaGameEvent& e)
{
    LOG_EVENT("Start Game, first to play: ", e.firstToPlayId,
             "Last card in deck: ", Cards::CardToString(e.lastCard));
}

void Logger::LogEv(const StartMatchEvent& e)
{
    LOG_EVENT("Start Match (", e.gameType, "), first to play: ", e.firstToPlayId);
}

void Logger::LogEv(const RoundOverEvent& e)
{
    LOG_EVENT("Round over, result: winnerId:", e.roundResult.winnerId, ", points: (", e.roundResult.points.punta, ", ", e.roundResult.points.bella, ")");
}
void Logger::LogEv(const GameOverEvent& e)
{
    LOG_EVENT("Game over");
}
void Logger::LogEv(const MatchOverEvent& e)
{
    LOG_EVENT("Match over");
}
void Logger::LogEv(const YourTurnEvent& e)
{
    LOG_EVENT("Your turn: ", e.playerId);
}
void Logger::LogEv(const MoveResponseEvent& e)
{
    LOG_EVENT("Move: ", e.moveValidity);
}

void Logger::LogEv(const AcussoEvent& e)
{
    std::string acussosString;
    for(auto& acusso : e.acussos)
    {
        if(!acussosString.empty())
            acussosString += ", ";
        acussosString.append(Acussos_NS::AcussoToString(acusso));
    }

    LOG_EVENT("Player: ", e.playerId.second, ", Acussos: ", acussosString);
}

void Logger::LogEv(const BriscolaLastRoundEvent& e)
{
    LOG_EVENT("BriscolaLastRoundEvent: receiver: ", e.receiverPlayerId,
            ", teammate: ", e.senderTeammatePlayerId,
            ", team points: ", e.teamPoints,
            ", teammateHand: ");
    Cards::logCards(e.senderTeammateHand);
}

void Logger::LogEv(const BeforeFirstMoveEvent& e)
{
}