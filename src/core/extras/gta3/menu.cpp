/*
 * Copyright (C) 2014  LINK/2012 <dma_2012@hotmail.com>
 * Licensed under GNU GPL v3, see LICENSE at top level directory.
 * 
 *      Mod Loader In-Game Menu
 *
 */
#include <stdinc.hpp>
#include "menu.hpp"
#include "loader.hpp"
#include <modloader/gta3/fxt.hpp>
using namespace modloader;

static CMenuItem StaticPages[] = 
{
    // Action, Name, Type, TargetMenu, RX, RY, Align

    // Main Page
	{   "ML_F0HH", -1, 0,
            MENU_ACTION_DUMMY,      "ML_F0EL",      MENU_ENTRY_OPTION,    -1,  57, 100,  1,
            MENU_ACTION_DUMMY,      "ML_F0LF",      MENU_ENTRY_OPTION,    -1,   0,   0,  1,
            MENU_ACTION_DUMMY,      "ML_F0LS",      MENU_ENTRY_OPTION,    -1,   0,   0,  1,
            MENU_ACTION_DUMMY,      "ML_F0RR",      MENU_ENTRY_BUTTON,    -1,   0,   0,  1,
            MENU_ACTION_DUMMY,      "ML_F0RA",      MENU_ENTRY_OPTION,    -1,   0,   0,  1,
            MENU_ACTION_SWITCH,     "ML_F0MM",      MENU_ENTRY_BUTTON,     1,   0,   0,  1,
            
            MENU_ACTION_BACK,       "ML_FTB",       MENU_ENTRY_BUTTON,    -1, 490, 380,  1,
    },
    
    // Modifications Page
	{   "ML_FMHH", 0, 0,
            MENU_ACTION_SWITCH,     "ML_FMY1",      MENU_ENTRY_BUTTON,     2,  30,  90,  1,
            MENU_ACTION_SWITCH,     "ML_FMY2",      MENU_ENTRY_BUTTON,     2,   0,   0,  1,
            MENU_ACTION_SWITCH,     "ML_FMY3",      MENU_ENTRY_BUTTON,     2,   0,   0,  1,
            MENU_ACTION_SWITCH,     "ML_FMY4",      MENU_ENTRY_BUTTON,     2,   0,   0,  1,
            MENU_ACTION_SWITCH,     "ML_FMY5",      MENU_ENTRY_BUTTON,     2,   0,   0,  1,
            MENU_ACTION_SWITCH,     "ML_FMY6",      MENU_ENTRY_BUTTON,     2,   0,   0,  1,
            MENU_ACTION_SWITCH,     "ML_FMY7",      MENU_ENTRY_BUTTON,     2,   0,   0,  1,
            MENU_ACTION_SWITCH,     "ML_FMY8",      MENU_ENTRY_BUTTON,     2,   0,   0,  1,
            MENU_ACTION_SWITCH,     "ML_FMY9",      MENU_ENTRY_BUTTON,     2,   0,   0,  1,

            MENU_ACTION_DUMMY,      "ML_FTP",       MENU_ENTRY_BUTTON,    -1,  30, 380,  1,
            MENU_ACTION_BACK,       "ML_FTB",       MENU_ENTRY_BUTTON,    -1, 320, 380,  3,
            MENU_ACTION_DUMMY,      "ML_FTN",       MENU_ENTRY_BUTTON,    -1, 500, 380,  1,
    },

    // Selected Modification Page
	{   "ML_FYHH", 1, 0,
            MENU_ACTION_DUMMY,      "ML_FYE0",      MENU_ENTRY_OPTION,    -1,  60,  110,  1,
            MENU_ACTION_DUMMY,      "ML_FYPR",      MENU_ENTRY_OPTION,    -1,   0,   0,  1,

            MENU_ACTION_BACK,       "ML_FTB",       MENU_ENTRY_BUTTON,    -1, 490, 380,  1,
    },

};

//
static const auto NumStaticPages = std::extent<decltype(StaticPages)>::value;   // Number of pages in the StaticPages array
static const auto NumPages = NumStaticPages;                                    // Number of pages we'll need to register
static const auto NumModsPerPage = 9;                                           // Number of mods in a single dynamic page (ML_FMHH)


/*
    TheMenu
        Our Mod Loader In-Game Menu
*/
class TheMenu : public AbstractFrontend
{
    private:
        fxt_manager     fxt;            // Fake GXT Manager

        CMenuItem*      pOptions;       // The options menu page
        CMenuEntryData* pEntry;         // The entry at the options menu we are in

        CMenuItem*      pPages;         // Our allocated pages array
        MenuPage        mPageMain;      // The "Mod Loader" page
        MenuPage        mPageMods;      // The "Modifications" page
        MenuPage        mPageMod;       // The "The Mod Config" page

        std::pair<std::string, delayed_message> mSearchText;    // The search text at the mods page
        ref_list<Loader::ModInformation>    mMods;              // List of mods being shown in the mods page
        int                                 mCurrentModsPage;   // 0 based index of the current page in the mods page (dynamic)
        ref_list<Loader::ModInformation>    mCurrentPageMods;   // List of mods in the current page
        Loader::Journal                     mChangedMods;       // Mods that have changed from the current menu usage

        // Data related to the "The Mod Config" page
        Loader::ModInformation*             mWorkingMod;        // The mod related to this page
        bool                                mWorkingModEnabled; // ENABLED state
        uint32_t                            mWorkingModPriority;// PRIORITY state
        
    public:
        TheMenu();

    private:
        // Initializers
        void LoadText();
        void RegisterMenu();
        void RegisterEvents();

        // Events Processing
        void MainPageEvents();
        void ModsPageEvents();
        void ModPageEvents();

        // Mods Page
        void RetrieveModsList();
        void RetrieveModsList(const std::string& keywords);
        bool BuildCurrentModsPage(int inc = 0);
        void ProcessModsSearch();
        void ModsPageNext();
        void ModsPagePrev();
        int  GetTotalModsPage();
        int  NumModsOnPage(int n);

        // Config Saving
        static void SaveBasicConfig(MenuEntry&);
        static bool RescanMods(ActionInfo&);
        
        // Writes temporary @text into the label @tmplabel and returns it's entry
        TextEntry TempTextLabel(const std::string& text, const char* tmplabel)
        {
            fxt.set(tmplabel, text.data());
            return TextLabel(tmplabel);
        }

        // Replaces the 'F' with a 'H' on the fourth character of the label and finds it's entry
        TextEntry HelpLabel(const std::string& label)
        {
            auto help = TextLabel(label);
            help.first[3] = 'H';
            return help;
        }
};


/*
    TheMenu
*/
TheMenu::TheMenu()
{
    mCurrentModsPage = 0;
    this->fxt.make_samp_compatible();
    this->LoadText();
    this->RegisterMenu();
    this->RegisterEvents();
}

/*
    TheMenu::SaveBasicConfig
        Saves the basic configuration file, essentialy forwards the call to the Loader
*/
void TheMenu::SaveBasicConfig(MenuEntry&)
{
    loader.SaveBasicConfig();
}

/*
    TheMenu::RescanMods
        Scan and update modifications
*/
bool TheMenu::RescanMods(ActionInfo&)
{
    loader.ScanAndUpdate();
    return true;
}



/*
    TheMenu::RegisterMenu
        Registers our menu into the game
*/
void TheMenu::RegisterMenu()
{
    // Allocate and setup array of pages
    this->pPages = RegisterMenuPage(NumStaticPages);
    std::memcpy(pPages, StaticPages, sizeof(StaticPages));
    this->RelativeSetup(pPages, NumStaticPages);

    // 
    this->mPageMain.Initialise(GetPage("ML_F0HH", pPages, NumPages));
    this->mPageMods.Initialise(GetPage("ML_FMHH", pPages, NumPages));
    this->mPageMod.Initialise(GetPage("ML_FYHH", pPages, NumPages));

    // Add our menu after the Options menu
    this->pOptions = GetPage("FET_OPT");
    this->pEntry = AddTargetMenuBefore(pOptions, GetEntry(pOptions, "FEDS_TB"), this->mPageMain, "ML_FEO");
}

/*
    TheMenu::RegisterEvents
        Registers events for button clicks etc
*/
void TheMenu::RegisterEvents()
{
    this->MainPageEvents();
    this->ModsPageEvents();
    this->ModPageEvents();
}


/*
    TheMenu::LoadText
        Loads the menu locale text
*/
void TheMenu::LoadText()
{
    // Mapping between the game language IDs and Locale IDs
    // static const uint32_t default_langs[] = { 1033, 1036, 1031, 1040, 1034 };
    // static const auto default_langs_len   = std::extent<decltype(default_langs)>::value;

    auto LoadFXT = [this](uint32_t locale)
    {
        auto lang = loader.gamePath + loader.dataPath + "/text/" + std::to_string(locale) + "/menu.fxt";
        return ParseFXT(fxt, lang.data());
    };

    this->fxt.add("ML__SM", "");    // Typed search text

    // Try to load a fxt with the OS locale
    auto locale = GetUserDefaultLCID();
    if(!LoadFXT(locale))
    {
        // Nope, try american fxt
        if(!LoadFXT(1033))
            loader.Log("Failed to load fallback menu fxt");
        else
            loader.Log("Loaded fallback menu fxt");
    }
    else loader.Log("Loaded menu fxt for locale %d", locale);
}









/*
    TheMenu::RetrieveModsList
        Retrieve the list of mods sorted by name
*/
void TheMenu::RetrieveModsList()
{
    this->mMods = loader.mods.GetModsByName();
}

/*
    TheMenu::RetrieveModsList
        Retrieve the list of mods that contains the keywords at @keyword_string sorted by name
*/
void TheMenu::RetrieveModsList(const std::string& keywords_string)
{
    // Find list of keywords at the keywords string, separated in a vector... Also lower case them for proper comparisions...
    auto keywords = modloader::split(keywords_string, ' ');
    std::for_each(keywords.begin(), keywords.end(), static_cast<std::string&(*)(std::string&)>(tolower));

    // If no keywords, retrieve all mods
    if(keywords.empty()) return this->RetrieveModsList();
    else this->mMods.clear();

    // Gets all mods........
    for(auto& mod : loader.mods.GetModsByName())
    {
        bool hasKeywords = true;
        for(auto& kw : keywords)
        {
            // ..... check if it's name contain the keywords .....
            if(mod.get().GetName().find(kw) == std::string::npos)
            {
                hasKeywords = false;
                break;
            }
        }

        if(hasKeywords) mMods.emplace_back(mod);
    }
}

/*
    TheMenu::GetTotalModsPage
        Finds the total number of pages in the dynamic mods page
*/
int TheMenu::GetTotalModsPage()
{
    if(mMods.size())
    {
        auto div = std::div(mMods.size(), NumModsPerPage);
        return div.rem? div.quot + 1 : div.quot;
    }
    return 0;
}

/*
    TheMenu::NumModsOnPage
        Finds number of mods on the mods page of index @n in the dynamic mods page
*/
int TheMenu::NumModsOnPage(int n)
{
    if(n >= 0)
    {
        if(auto max = GetTotalModsPage())
        {
            if(n + 1 >= max)    // last page?
            {
                auto rem = (mMods.size() % NumModsPerPage);
                return rem? rem : NumModsPerPage;
            }
            return NumModsPerPage;
        }
    }
    return 0;
}


/*
    TheMenu::BuildCurrentModsPage
        Retrieve the list of mods that contains the keywords at @keyword_string sorted by name
*/
bool TheMenu::BuildCurrentModsPage(int inc)
{
    this->mCurrentPageMods.clear();

    if(auto total = GetTotalModsPage())
    {
        // Increase the current page by inc (-1 or 1)
        this->mCurrentModsPage += inc;

        //  Check if page is in rage
        if(mCurrentModsPage < 0) mCurrentModsPage = total - 1;
        else if(mCurrentModsPage >= total) mCurrentModsPage = 0;

        // Rebuild the menu entries
        for(int i = 0, numInPage = NumModsOnPage(mCurrentModsPage); i < NumModsPerPage; ++i)
        {
            auto& entry = mPageMods.GetEntry(i)->pEntry;
            auto  skip  = (i >= numInPage);                 // Should skip this entry? (i.e. no more mods for another entry)
            entry->m_nActionType = skip? MENU_ACTION_SKIP : MENU_ACTION_SWITCH;

            if(!skip)
            {
                auto& modinfo = this->mMods[mCurrentModsPage * NumModsPerPage + i].get();
                mCurrentPageMods.emplace_back(modinfo);
                fxt.set(entry->m_szName, modinfo.GetName().c_str());
            }
            else
                fxt.set(entry->m_szName, "");
            
        }
        return true;
    }
    else
    {
        this->mCurrentModsPage = 0;

        for(int i = 0; i < NumModsPerPage; ++i)
        {
            auto& entry = mPageMods.GetEntry(i)->pEntry;
            entry->m_nActionType = MENU_ACTION_SKIP;
            fxt.set(entry->m_szName, "");
        }

        return false;
    }
}

/*
    TheMenu::ModsPageNext
        Goes to the next page on the dynamic mods page
*/
void TheMenu::ModsPageNext()
{
    this->ReportHighlight();
    this->BuildCurrentModsPage(1);
}

/*
    TheMenu::ModsPagePrev
        Goes to the previous page on the dynamic mods page
*/
void TheMenu::ModsPagePrev()
{
    this->ReportHighlight();
    this->BuildCurrentModsPage(-1);
}

/*
    TheMenu::ProcessModsSearch
        Processes the searching action in the dynamic mods page
*/
void TheMenu::ProcessModsSearch()
{
    static const ms_t delay(2000);    // How much time until the typed text becomes invalid?

    // Fetch any textual input
    auto backspace = HasJustPressedBackspace();
    auto input     = FetchInputText();

    // If the typed text became invalid clear it
    if(mSearchText.second.has_completed())
        mSearchText.first.clear();

    // Do "heavy" processing only if any textual input
    if(input.size() || backspace)
    {
        // Reflect the input in the search text buffer
        if(input.size())
            mSearchText.first.append(tolower(input));
        else if(backspace && mSearchText.first.size())
            mSearchText.first.pop_back();

        // Find the time for the text to become invalid
        mSearchText.second = delayed_message(delay);

        // Print the typed text, search mods with it and then rebuild the page 
        this->SetHelperText(TempTextLabel(mSearchText.first + '_', "ML__SM"), nullptr, -1);
        this->RetrieveModsList(mSearchText.first);
        this->BuildCurrentModsPage();
    }
}








/*
    TheMenu::MainPageEvents
        Main page events registering and processing
        This page is at the Options menu of the game
*/
void TheMenu::MainPageEvents()
{
    // Helper function to setup a boolean entry in the menu
    auto SetupBooleanEntry2 = [this](const char* label, bool& boolean, std::function<void(MenuEntry&)> cb)
    {
        if(auto entry = mPageMain.GetEntry(label))
        {
            entry->SetHelper(HelpLabel(label));
            entry->OnStateChange(cb);
            OnAction(this->pOptions, this->pEntry, entry->SetupBooleanEntry(std::ref(boolean)));
        }
    };

    // Helper function to setup a boolean entry in the menu
    auto SetupBooleanEntry = [this, SetupBooleanEntry2](const char* label, bool& boolean)
    {
        return SetupBooleanEntry2(label, boolean, SaveBasicConfig);
    };

    // Helper function to setup a integer MegaByte entry in the menu
    auto SetupMegaByteEntry = [this](const char* label, uint64_t& size)
    {
        static const uint64_t mb = 1048576;
        static const uint64_t min = mb, max = mb * 100, step = mb;

        // Helper function to find the "X MB" text from the number of bytes
        auto MegaByteLabel = [](const uint64_t& bytes)
        {
            auto text = NumberEntry(bytes / mb);
            text.first.append("MB");
            return text;
        };

        if(auto entry = mPageMain.GetEntry(label))
        {
            entry->SetHelper(HelpLabel(label));
            entry->OnStateChange(SaveBasicConfig);

            // This one looks a bit of a complex call, but it's simple and flexible
            // (reference to the state variable, state_to_text_functor, change state from action functor)
            // Returns another functor, to be called when the page the entry is in should be setuped
            auto fInitStates = entry->SetupStatefulEntry(std::ref(size), std::ref(MegaByteLabel), [](const uint64_t& value, ActionInfo& info)
            {
                return std::min(std::max((value + step * info.wheel), min), max);
            });

            OnAction(this->pOptions, this->pEntry, std::move(fInitStates));
        }
    };

    // Rescan modifications
    if(auto entry = this->mPageMain.GetEntry("ML_F0RR"))
    {
        OnAction(this->mPageMain, *entry, [this](ActionInfo& info) {
            this->mChangedMods.clear();
            return this->RescanMods(info);
        });
        entry->SetHelper(HelpLabel("ML_F0RR"));
    }


    // When entering the menu pause the FS watcher so we don't come up with any surprise
    OnAction(this->pOptions, this->pEntry, [this](ActionInfo& info)
    {
        if(info.enter)
        {
            loader.PauseWatcher();
            this->mChangedMods.clear();
        }
        return true;
    });

    // When getting out of the menu we have to do some changes to the modloader state
    OnUserInput(this->mPageMain, [this](UserInputInfo& info)
    {
        if(info.exit)
        {
            if(this->mChangedMods.size())
            {
                loader.UpdateFromJournal(this->mChangedMods);
                this->mChangedMods.clear();
            }


            // Reset the filesystem watcher so it reflects the new bAutoRefresh state
            loader.RestartWatcher();

        }
        return true;
    });

    SetupBooleanEntry("ML_F0E0", loader.bEnableMenu);
    SetupBooleanEntry("ML_F0EP", loader.bEnablePlugins);
    SetupBooleanEntry("ML_F0EL", loader.bEnableLog);
    SetupBooleanEntry("ML_F0LF", loader.bImmediateFlush);
    SetupBooleanEntry("ML_F0RA", loader.bAutoRefresh);
    SetupMegaByteEntry("ML_F0LS", loader.maxBytesInLog);
}

/*
    TheMenu::ModsPageEvents
        Dynamic mods page events registering and processing
        This page is at the "Mod Loader > Modifications"
*/
void TheMenu::ModsPageEvents()
{
    // On click "Modifications" to open the modifications page...
    OnAction(this->mPageMain, *mPageMain.GetEntry("ML_F0MM"), [this](ActionInfo& info)
    {
        this->RetrieveModsList();
        if(!this->BuildCurrentModsPage())
        {
            // Ops... no mods installed probably
            this->ReportError(TextLabel("ML_H0NM"));
            return false;
        }
        this->SetHelperText(TextLabel("ML_HMHH"), this->mPageMods, -1);
        return true;
    });

    // Inputs in this modifications page
    OnUserInput(this->mPageMods, [this](UserInputInfo& info)
    {
        // Process mods searching by typing
        this->ProcessModsSearch();

        // Change page if pressed left/right
        if(info.wheel > 0)      this->ModsPageNext();
        else if(info.wheel < 0) this->ModsPagePrev();

        // Continue processing input only if no page change
        return info.wheel == 0;
    });

    // Next page button
    OnAction(this->mPageMods, *mPageMods.GetEntry("ML_FTN"), [this](ActionInfo& info)
    {
        this->ModsPageNext();
        return true;
    });

    // Prev page button
    OnAction(this->mPageMods, *mPageMods.GetEntry("ML_FTP"), [this](ActionInfo& info)
    {
        this->ModsPagePrev();
        return true;
    });
}

/*
    TheMenu::ModsPageEvents
        Dynamic mod page events registering and processing
        This page is at the "Mod Loader > Modifications > Some Mod"
*/
void TheMenu::ModPageEvents()
{
    // Gets the MenuEntry from the specified index
    auto EntryFromIndex = [this](int i)
    {
        return mPageMods.GetEntry(std::string("ML_FMY").append(std::to_string(i)).data());
    };

    // Helper function to setup a boolean entry in the menu
    auto SetupBooleanEntry = [this](const char* label, bool& boolean, std::function<void(MenuEntry&)> cb) -> std::function<bool(ActionInfo&)>
    {
        if(auto entry = mPageMod.GetEntry(label))
        {
            entry->SetHelper(HelpLabel(label));
            entry->OnStateChange(cb);
            return entry->SetupBooleanEntry(std::ref(boolean)); // returns a functor (UpdateState())
        }
        return nullptr;
    };

    // Helper function to setup a integer priority entry in the menu
    auto SetupPriorityEntry = [this](const char* label, uint32_t& priority, std::function<void(MenuEntry&)> cb) -> std::function<bool(ActionInfo&)>
    {
        static const uint32_t min = 0, max = 100, step = 1;

        auto PriorityLabel = [](const uint32_t& priority)
        {
            auto text = NumberEntry(priority);
            text.first.push_back('_');
            return text;
        };

        if(auto entry = mPageMod.GetEntry(label))
        {
            entry->SetHelper(HelpLabel(label));
            entry->OnStateChange(cb);

            auto fInitStates = entry->SetupStatefulEntry(std::ref(priority), std::ref(PriorityLabel), [](const uint32_t& value, ActionInfo& info)
            {
                return std::min(std::max((value + step * info.wheel), min), max);
            });

            return fInitStates;
        }
        return nullptr;
    };




    // ENABLED state
    auto vEnabledUpdate = SetupBooleanEntry("ML_FYE0", mWorkingModEnabled, [this](MenuEntry&)
    {
        if(mWorkingModEnabled) mWorkingMod->Parent().Profile().UnignoreMod(mWorkingMod->GetName());
        else mWorkingMod->Parent().Profile().IgnoreMod(mWorkingMod->GetName());
        
        mChangedMods.emplace(mWorkingMod->GetName(), Loader::Status::Updated);
        loader.SaveFolderConfig();
    });

    // PRIORITY state
    auto vPriorityUpdate = SetupPriorityEntry("ML_FYPR", mWorkingModPriority, [this](MenuEntry&)
    {
        mWorkingMod->Parent().Profile().SetPriority(mWorkingMod->GetName(), mWorkingModPriority);
        mChangedMods.emplace(mWorkingMod->GetName(), Loader::Status::Updated);
        loader.SaveFolderConfig();
    });

    // Builds the page for the specified mod
    auto BuildModPage = [=](ActionInfo& info, Loader::ModInformation& mod)
    {
        static const size_t max_title_size = 17;

        // Setup mod info
        this->mWorkingMod = &mod;
        this->mWorkingModEnabled = !(this->mWorkingMod->Parent().Profile().IsOnIgnoringList(this->mWorkingMod->GetName()));
        this->mWorkingModPriority = this->mWorkingMod->Parent().Profile().GetPriority(this->mWorkingMod->GetName());
        vEnabledUpdate(info);   // Updates the state text for the entry
        vPriorityUpdate(info);  // Updates the state text for the entry

        // The title
        std::string title = this->mWorkingMod->GetName();
        if(title.size() >= max_title_size)
        {
            title.resize(max_title_size - 3);
            title.append("...");
        }
        fxt.set("ML_FYHH", modloader::toupper(title).data());
    };

    // Mod page builder for each entry in the Mods page
    for(int i = 1; auto entry = EntryFromIndex(i); ++i)
    {
        OnAction(this->mPageMods, *entry, [=](ActionInfo& info)
        {
            auto index = size_t(i-1);
            if(this->mCurrentPageMods.size() > index)
            {
                BuildModPage(info, this->mCurrentPageMods[index]);
                return true;
            }
            return false;
        });
    }
}









/*
    Loader::StartupMenu 
    Loader::ShutdownMenu
        Interactor between the main loader object and TheMenu
        Starts / Shutdowns the menu
*/

static std::unique_ptr<TheMenu> menu_ptr;

void Loader::StartupMenu()
{
    if(try_address(0x8CE008))   // aScreens
    {
        if(loader.bEnableMenu)
            menu_ptr.reset(new TheMenu());
        else
            Log("Menu disabled, not injecting one");
    }
    else
        Log("No aScreen address, not injecting a menu");
}

void Loader::ShutdownMenu()
{
    Log("Shutting down menu...");
    menu_ptr.reset();
}
