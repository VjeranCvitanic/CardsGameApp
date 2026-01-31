#pragma once
#include "CardsRound.h"

class BriscolaRules final : public CardsRound_NS::RoundRules
{
public:
    static BriscolaRules& instance()
    {
        static BriscolaRules instance;
        return instance;
    }

    // delete copy & move
    BriscolaRules(const BriscolaRules&) = delete;
    BriscolaRules& operator=(const BriscolaRules&) = delete;
    BriscolaRules(BriscolaRules&&) = delete;
    BriscolaRules& operator=(BriscolaRules&&) = delete;

    int8_t getNumberStrength(Number number) const override;
    Points getNumberValue(Number number) const override;
    Card StrongerCard(const Card& card1,
                      const Card& card2,
                      Color strongColor) const override;
    bool IsMoveLegal(const Move&,
                     const CardsRound_NS::RoundState& state,
                     MoveReturnValue& reason) const override;

private:
    BriscolaRules() = default; // private ctor
};
