
enum class AchievementMenuState
{
    Transition,
    Achievements,
    ClosingAchievements
};


class AchievementMenu
{
public:
    static inline AchievementMenuState s_state{};
    static inline AchievementMenuState s_nextState{};
    static inline bool s_isVisible = false;
	static inline float g_BackgroundAnimIntro = -1.0f;
	static inline float g_BackgroundAnimClose = -1.0f;

    static void Init();
    static void Draw();
    static void Open();
    static void Close();
    static void SetState(AchievementMenuState state);
};
