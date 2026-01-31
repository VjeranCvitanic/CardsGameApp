#pragma once
#include "CardsRound.h"

class TressetteRules final : public CardsRound_NS::RoundRules
{
public:
    static TressetteRules& instance()
    {
        static TressetteRules instance;
        return instance;
    }

    // delete copy & move
    TressetteRules(const TressetteRules&) = delete;
    TressetteRules& operator=(const TressetteRules&) = delete;
    TressetteRules(TressetteRules&&) = delete;
    TressetteRules& operator=(TressetteRules&&) = delete;

    int8_t getNumberStrength(Number number) const override;
    Points getNumberValue(Number number) const override;
    Card StrongerCard(const Card& card1,
                      const Card& card2,
                      Color strongColor) const override;
    bool IsMoveLegal(const Move&,
                     const CardsRound_NS::RoundState& state,
                     MoveReturnValue& reason) const override;
    bool checkConstraints(const CardSet& hand, Card card, Color leadColor) const;

private:
    TressetteRules() = default; // private ctor
};
