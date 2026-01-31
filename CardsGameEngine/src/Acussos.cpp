#include "Acussos.h"
#include "Cards.h"

std::string Acussos_NS::AcussoToString(AcussoType a)
{
    switch (a)
    {
        case AcussoType::NapolitanaSpade:   return "Napolitana di spade";
        case AcussoType::NapolitanaCoppe:   return "Napolitana di coppe";
        case AcussoType::NapolitanaDenari:  return "Napolitana di denari";
        case AcussoType::NapolitanaBastoni: return "Napolitana di bastoni";
        case AcussoType::AssoAcusso:        return "Asso d'acusso";
        case AcussoType::DueAcusso:         return "Due d'acusso";
        case AcussoType::TreAcusso:         return "Tre d'acusso";
        case AcussoType::AssoSenzaSpade:    return "Asso senza spade";
        case AcussoType::AssoSenzaCoppe:    return "Asso senza coppe";
        case AcussoType::AssoSenzaDenari:   return "Asso senza denari";
        case AcussoType::AssoSenzaBastoni:  return "Asso senza bastoni";
        case AcussoType::DueSenzaSpade:     return "Due senza spade";
        case AcussoType::DueSenzaCoppe:     return "Due senza coppe";
        case AcussoType::DueSenzaDenari:    return "Due senza denari";
        case AcussoType::DueSenzaBastoni:   return "Due senza bastoni";
        case AcussoType::TreSenzaSpade:     return "Tre senza spade";
        case AcussoType::TreSenzaCoppe:     return "Tre senza coppe";
        case AcussoType::TreSenzaDenari:    return "Tre senza denari";
        case AcussoType::TreSenzaBastoni:   return "Tre senza bastoni";
        case AcussoType::NoAcusso:           return "Nessun acusso";
    }
    return "Acusso sconosciuto";
}

int Acussos_NS::AcussoCheck(const CardSet& hand, Acussos& acussosList)
{
    int points = 0;
    CalculateAcussoPoints(hand, points, acussosList);

    return points;
}

void Acussos_NS::CalculateAcussoPoints(const CardSet& hand, int& points, Acussos& Acussos)
{
    points = 0;
    points += Napolitana(hand, Acussos);
    points += SameNumberAcusso(hand, Acussos);
}

int Acussos_NS::SameNumberAcusso(const CardSet& hand, Acussos& Acussos)
{
    int pts = 0;
    for(int n = Asso; n <= Tre; n++)
    {
        Color senza = NoColor;
        bool flag = true;
        for(int c = Spade; c <= Bastoni; c++)
        {
            if(!Cards::isCardInCardSet(hand, Cards::makeCard(static_cast<Color>(c), static_cast<Number>(n))))
            {
                if(senza == NoColor)
                {
                    senza = static_cast<Color>(c);
                }
                else {
                    flag = false;
                    break;
                }
            }
        }
        if(flag && senza == NoColor)
        {
            pts += 4;
            switch(n)
            {
                case Asso:
                    Acussos.push_back(AssoAcusso);
                    break;
                case Due:
                    Acussos.push_back(DueAcusso);
                    break;
                case Tre:
                    Acussos.push_back(TreAcusso);
                    break;
                default:
                    break;
            }
        }
        else if(flag)
        {
            pts += 3;
            switch(n)
            {
                case Asso:
                    switch(senza)
                    {
                        case Spade:
                            Acussos.push_back(AssoSenzaSpade);
                            break;
                        case Coppe:
                            Acussos.push_back(AssoSenzaCoppe);
                            break;
                        case Denari:
                            Acussos.push_back(AssoSenzaDenari);
                            break;
                        case Bastoni:
                            Acussos.push_back(AssoSenzaBastoni);
                            break;
                        default:
                            break;
                    }
                    break;
                case Due:
                    switch(senza)
                    {
                        case Spade:
                            Acussos.push_back(DueSenzaSpade);
                            break;
                        case Coppe:
                            Acussos.push_back(DueSenzaCoppe);
                            break;
                        case Denari:
                            Acussos.push_back(DueSenzaDenari);
                            break;
                        case Bastoni:
                            Acussos.push_back(DueSenzaBastoni);
                            break;
                        default:
                            break;
                    }
                    break;
                case Tre:
                    switch(senza)
                    {
                        case Spade:
                            Acussos.push_back(TreSenzaSpade);
                            break;
                        case Coppe:
                            Acussos.push_back(TreSenzaCoppe);
                            break;
                        case Denari:
                            Acussos.push_back(TreSenzaDenari);
                            break;
                        case Bastoni:
                            Acussos.push_back(TreSenzaBastoni);
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
        }
    }
    return pts;
}

int Acussos_NS::Napolitana(const CardSet& hand, Acussos& Acussos)
{
    int pts = 0;
    if(hand.size() < 3)
    {
        return pts;
    }
    if(Cards::isCardInCardSet(hand, Cards::makeCard(Spade, Asso)) &&
       Cards::isCardInCardSet(hand, Cards::makeCard(Spade, Due)) &&
       Cards::isCardInCardSet(hand, Cards::makeCard(Spade, Tre)))
    {
        Acussos.push_back(NapolitanaSpade);
        pts += 3;
    }
    if(Cards::isCardInCardSet(hand, Cards::makeCard(Denari, Asso)) &&
       Cards::isCardInCardSet(hand, Cards::makeCard(Denari, Due)) &&
       Cards::isCardInCardSet(hand, Cards::makeCard(Denari, Tre)))
    {
        Acussos.push_back(NapolitanaDenari);
        pts += 3;
    }
    if(Cards::isCardInCardSet(hand, Cards::makeCard(Bastoni, Asso)) &&
       Cards::isCardInCardSet(hand, Cards::makeCard(Bastoni, Due)) &&
       Cards::isCardInCardSet(hand, Cards::makeCard(Bastoni, Tre)))
    {
        Acussos.push_back(NapolitanaBastoni);
        pts += 3;
    }
    if(Cards::isCardInCardSet(hand, Cards::makeCard(Coppe, Asso)) &&
       Cards::isCardInCardSet(hand, Cards::makeCard(Coppe, Due)) &&
       Cards::isCardInCardSet(hand, Cards::makeCard(Coppe, Tre)))
    {
        Acussos.push_back(NapolitanaCoppe);
        pts += 3;
    }
    return pts;
}
