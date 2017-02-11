#ifndef H_ANTICHEAT_H
#define H_ANTICHEAT_H

#include "WorldSession.h"
#include "Vehicle.h"

class Player;

class AntiCheat
{
    public:
        AntiCheat(Player* player) : _Player(player)
        {
            _MovementInfo   = nullptr;
            _Mover          = nullptr;
            _Opcode         = 0;
            _LastClientTime = 0;
            _Latency        = 0;

            _Distance2D     = 0.0f;
            _Distance3D     = 0.0f;
            _LastSpeed      = 0.0f;

            _AlreadyJumped  = false;
            _OnlyAlarm      = false;
        }

        bool MovementCheck(MovementInfo* _MovementInfo, uint16 Opcode);
        void MovementCorrection();
        void SendAlarm();

    private:
        MovementInfo* _MovementInfo;
        Player* _Player;
        Unit* _Mover;

        uint16 _Opcode;
        uint32 _LastClientTime, _Latency;

        float _Distance2D, _Distance3D, _LastSpeed;

        bool _AlreadyJumped;
        bool _OnlyAlarm;
        bool IsSpeedOkay();
        bool IsJumpOkay();

        UnitMoveType GetMovementType();
};

#endif
