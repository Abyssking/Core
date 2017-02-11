#include "Player.h"
#include "AntiCheat.h"
#include "Creature.h"
#include "Chat.h"

bool AntiCheat::MovementCheck(MovementInfo* MoveInfo, uint16 Opcode)
{
    if (!sWorld->getBoolConfig(CONFIG_ANTI_CHEAT))
        return false;

    if (_Player->GetSession()->GetSecurity() > 0)
        return false;

    if (_Player->GetCharmerOrOwnerPlayerOrPlayerItself()->GetVehicle())
        return false;

    if (_Player->HasAura(63163) || _Player->HasAura(51852))
        return false;

    if (_Player->InArena() || _Player->InBattleground() || _Player->GetMap()->IsRaid())
        _OnlyAlarm = true;

    _Opcode = Opcode;
    _MovementInfo = MoveInfo;
    _Mover = _Player->m_unitMovedByMe;
    _Distance2D = _Mover->GetExactDist2d(_MovementInfo->pos.m_positionX, _MovementInfo->pos.m_positionY);
    _Distance3D = _Mover->GetExactDist(_MovementInfo->pos.m_positionX, _MovementInfo->pos.m_positionY, _MovementInfo->pos.m_positionZ);

    if (Player* Mover = _Mover->ToPlayer())
        _Latency = Mover->GetSession()->GetLatency();
    else
        _Latency = _Player->GetSession()->GetLatency();

    if (!(_Player->HasAuraType(SPELL_AURA_FLY) || _Player->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED) || _Player->HasAuraType(SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED)))
        _Player->SetCanFly(false);

    if (!IsSpeedOkay())
        return true;

    if (!IsJumpOkay())
        return true;

    return false;
}

void AntiCheat::MovementCorrection()
{
    if (_OnlyAlarm)
        return;

    WorldPacket Data;
    _Mover->SetUnitMovementFlags(MOVEMENTFLAG_NONE);

    if (Player* Mover = _Mover->ToPlayer())
        if (Mover != _Player)
            Mover->SendTeleportAckPacket();

    _Player->SendTeleportAckPacket();
    _Mover->BuildHeartBeatMsg(&Data);
    _Mover->SendMessageToSet(&Data, true);
}

void AntiCheat::SendAlarm()
{
    std::string Message = _Player->GetName() + " Using Cheat";
    sWorld->SendGMText(6613, Message);
}

bool AntiCheat::IsSpeedOkay()
{
    if (_MovementInfo->flags & MOVEMENTFLAG_ONTRANSPORT)
    {
        _LastClientTime = _MovementInfo->time;
        return true;
    }

    const uint32 ClientTimeDelta = _MovementInfo->time - _LastClientTime + (_Latency * 3);
    const float CurrentSpeed = _Mover->GetSpeed(GetMovementType());
    const float TotalTimeDelta = ClientTimeDelta * 0.001f;
    const float SpeedToCheck = (_Opcode == MSG_MOVE_FALL_LAND || _Mover->HasUnitState(UNIT_STATE_CHARGING)) ? SPEED_CHARGE : ((CurrentSpeed < _LastSpeed) ? _LastSpeed : CurrentSpeed);
    const float AllowedDistance = (SpeedToCheck * TotalTimeDelta) + 0.015f + (_MovementInfo->jump.xyspeed / 2);
    const bool IsFlyIng = (_MovementInfo->flags & (MOVEMENTFLAG_FLYING)) != 0;

    _LastClientTime = _MovementInfo->time;
    _LastSpeed = CurrentSpeed;

    if ((IsFlyIng ? _Distance3D : _Distance2D) > AllowedDistance)
        return false;

    return true;
}

bool AntiCheat::IsJumpOkay()
{
    if (_Opcode != MSG_MOVE_JUMP || _Opcode == MSG_MOVE_FALL_LAND || _Player->IsInWater())
    {
        _AlreadyJumped = false;
        return true;
    }

    if (_AlreadyJumped)
        return false;

    _AlreadyJumped = true;

    return true;
}

UnitMoveType AntiCheat::GetMovementType()
{
    UnitMoveType MoveType;

    switch (_MovementInfo->flags & (MOVEMENTFLAG_FLYING | MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_WALKING))
    {
        case MOVEMENTFLAG_FLYING:
            MoveType = _MovementInfo->flags & MOVEMENTFLAG_BACKWARD ? MOVE_FLIGHT_BACK : MOVE_FLIGHT;
            break;
        case MOVEMENTFLAG_SWIMMING:
            MoveType = _MovementInfo->flags & MOVEMENTFLAG_BACKWARD ? MOVE_SWIM_BACK : MOVE_SWIM;
            break;
        case MOVEMENTFLAG_WALKING:
            MoveType = MOVE_WALK;
            break;
        default:
            MoveType = _MovementInfo->flags & MOVEMENTFLAG_BACKWARD ? MOVE_RUN_BACK : MOVE_RUN;
            break;
    }

    return MoveType;
}
