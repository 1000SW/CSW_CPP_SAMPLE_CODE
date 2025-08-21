#include "BehaviorTreeHeader.h"

std::string Convert_CharacterState_To_String(CharacterState eState)
{
    static const std::map<CharacterState, std::string> s_characterStateToStringMap = {
        {CS_DEAD, "CS_DEAD"},
        {CS_HIT, "CS_HIT"},
        {CS_INTERACTION, "CS_INTERACTION"},
        {CS_MOVE, "CS_MOVE"},
        {CS_IDLE, "CS_IDLE"}
    };

    auto it = s_characterStateToStringMap.find(eState);
    if (it != s_characterStateToStringMap.end())
    {
        return it->second;
    }

    // 유효하지 않은 열거형 값에 대한 처리
    std::cerr << "Warning: Unknown CharacterState enum value: " << static_cast<int>(eState) << std::endl;
    return "UNKNOWN_CHARACTER_STATE";
}

CharacterState Convert_String_To_CharacterState(const std::string& strStateName)
{
    static const std::map<std::string, CharacterState> s_stringToCharacterStateMap = {
        {"CS_DEAD", CS_DEAD},
        {"CS_HIT", CS_HIT},
        {"CS_INTERACTION", CS_INTERACTION},
        {"CS_MOVE", CS_MOVE},
        {"CS_IDLE", CS_IDLE}
    };

    std::string upperStr = strStateName;
    std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(), ::toupper);

    auto it = s_stringToCharacterStateMap.find(upperStr);
    if (it != s_stringToCharacterStateMap.end())
    {
        return it->second;
    }

    // 일치하는 문자열이 없을 경우 처리
    std::cerr << "Warning: Unknown CharacterState string: " << strStateName << std::endl;
    return CS_END; // 유효하지 않은 값임을 나타내기 위해 CS_END 반환
}
