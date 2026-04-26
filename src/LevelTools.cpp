#include <Geode/Geode.hpp>
#include <Geode/modify/EditLevelLayer.hpp>
#include <Geode/modify/LevelPage.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>
#include <Geode/modify/LevelAreaInnerLayer.hpp>
#include "Utils.hpp"

using namespace geode::prelude;

class $modify(MyEditLevelLayer, EditLevelLayer) {
    bool init(GJGameLevel* level) {
        if (!EditLevelLayer::init(level)) return false;

        bool copyLevelString = Mod::get()->getSettingValue<bool>("copy-level-string");

        if (copyLevelString) {
            auto menu = this->getChildByID("level-actions-menu");

            CCSprite* buttonSprite = CircleButtonSprite::create(
                CCLabelBMFont::create("Copy\nData", "bigFont.fnt", 0.f, CCTextAlignment::kCCTextAlignmentCenter)
            );
            CCMenuItemSpriteExtra* button = CCMenuItemSpriteExtra::create(
                buttonSprite,
                this,
                menu_selector(MyEditLevelLayer::copyLevel)
            );

            menu->addChild(button);
            menu->updateLayout();
        }

        return true;
    }

    void copyLevel(CCObject* sender) {
        std::string levelCopyType = Mod::get()->getSettingValue<std::string>("level-copy-type");
        std::string levelString = m_level->m_levelString;
        levelString = Utils::processLevelString(levelString);

        if (levelCopyType == "Encoded") {
            clipboard::write(levelString);
            Notification* n = Notification::create("Encoded level copied successfully!", NotificationIcon::Success);
            n->show();
        } else if (levelCopyType == "Decoded") {
            std::string decrypted = ZipUtils::decompressString(levelString, false, 1);
            clipboard::write(decrypted);
            Notification* n = Notification::create("Decoded level copied successfully!", NotificationIcon::Success);
            n->show();
        } else {
            Notification* n = Notification::create("Unknown level type", NotificationIcon::Error);
            n->show();
        }
    }
};

class $modify(MyLevelPage, LevelPage) {
    struct Fields {
        CCMenu* m_menu;
    };

    bool init(GJGameLevel* level) {
        if (!LevelPage::init(level)) return false;

        bool copyMainLevels = Mod::get()->getSettingValue<bool>("copy-main-levels");

        if (copyMainLevels) {
            CCSprite* buttonSprite = CircleButtonSprite::create(CCLabelBMFont::create("Copy\nLevel", "bigFont.fnt", 0.f, CCTextAlignment::kCCTextAlignmentCenter));
            CCMenuItemSpriteExtra* button = CCMenuItemSpriteExtra::create(
                buttonSprite,
                this,
                menu_selector(MyLevelPage::onCopyLevel)
            );

            CCSize winSize = CCDirector::sharedDirector()->getWinSize();
            m_fields->m_menu = CCMenu::createWithItem(button);
            m_fields->m_menu->setPositionX(winSize.width * 0.9f);
            this->addChild(m_fields->m_menu);
        }

        return true;
    }

    void onCopyLevel(CCObject* sender) {
        GameLevelManager* glm = GameLevelManager::sharedState();
        GameManager* gm = GameManager::sharedState();
        LocalLevelManager* llm = LocalLevelManager::sharedState();
        CCDirector* ccd = CCDirector::sharedDirector();

        if (ccd->getIsTransitioning()) return;
        this->setKeypadEnabled(false);

        // log::debug("Level string for \"{}\": {}", m_level->m_levelName, m_level->m_levelString);
        
        gm->m_sceneEnum = 2;
        GJGameLevel* level = glm->createNewLevel();
        level->copyLevelInfo(m_level);
        level->m_levelType = GJLevelType::Editor;
        std::string levelString = llm->getMainLevelString(m_level->m_levelID);
        levelString = Utils::processLevelString(levelString);
        level->m_levelString = levelString;

        CCScene* scene = EditLevelLayer::scene(level);
        CCTransitionFade* transition = CCTransitionFade::create(0.5f, scene);
        ccd->replaceScene(transition);
    }

    void updateDynamicPage(GJGameLevel* level) {
        LevelPage::updateDynamicPage(level);
        if (m_fields->m_menu) {
            if (level->m_levelID > 0) {
                m_fields->m_menu->setVisible(true);
            } else {
                m_fields->m_menu->setVisible(false);
            }
        }
    }
};

class $modify(MyEditorPauseLayer, EditorPauseLayer) {
    struct Fields {
        MDPopup* m_popup;
    };

    bool init(LevelEditorLayer* layer) {
        if (!EditorPauseLayer::init(layer)) return false;

        auto menu = this->getChildByID("guidelines-menu");
        bool copy = Mod::get()->getSettingValue<bool>("copy-level-string");
        bool paste = Mod::get()->getSettingValue<bool>("paste-level-string");

        if (copy) {
            CCSprite* buttonSprite = CircleButtonSprite::create(CCLabelBMFont::create("Copy\nData", "bigFont.fnt", 0.f, CCTextAlignment::kCCTextAlignmentCenter));
            CCMenuItemSpriteExtra* button = CCMenuItemSpriteExtra::create(
                buttonSprite,
                this,
                menu_selector(MyEditorPauseLayer::onCopyLevelString)
            );

            menu->addChild(button);
            menu->updateLayout();
        }
        if (paste) {
            CCSprite* buttonSprite = CircleButtonSprite::create(CCLabelBMFont::create("Paste\nLevel", "bigFont.fnt", 0.f, CCTextAlignment::kCCTextAlignmentCenter));
            CCMenuItemSpriteExtra* button = CCMenuItemSpriteExtra::create(
                buttonSprite,
                this,
                menu_selector(MyEditorPauseLayer::onPasteLevelString)
            );

            menu->addChild(button);
            menu->updateLayout();
        }

        return true;
    }

    void onPasteLevelString(CCObject* sender) {
        std::string levelString = clipboard::read();
        levelString = Utils::processLevelString(levelString);
        gd::string decompString = levelString;
        if (Utils::isCompressedLevel(decompString)) {
            decompString = ZipUtils::decompressString(decompString, false, 0);
        } else { // if you had like 1;2 for some reason instead on your clipboard
            levelString = ZipUtils::compressString(levelString, false, 0);
        };
        m_editorLayer->m_level->m_levelString = levelString;
        LevelSettingsObject* obj =  Utils::LevelSettingsObject(decompString);
        if (obj) {
            if (m_editorLayer->m_levelSettings) {
                m_editorLayer->m_levelSettings->release();
                m_editorLayer->m_levelSettings = nullptr;
            }
            obj->retain();
            obj->m_level = m_editorLayer->m_level; 
            m_editorLayer->m_levelSettings = obj;
            m_editorLayer->levelSettingsUpdated();
            m_editorLayer->loadLevelSettings();
        } else {
            log::error("failed to create LevelSettingsObject?");
        }

        // place the objects in the editor
        m_editorLayer->removeAllObjects();
        m_editorLayer->createObjectsFromString(decompString, true, true);

        MDPopup* popup = MDPopup::create(
            "Level Pasted",
            "Level pasted <cg>successfully</c>!\n\n"
            "<cr>Important:</c>\n"
            "- You must manually set song ID and some metadata as they are not included in the level string\n"
            "- Secret coins are automatically removed (game limitation)\n\n",
            "OK"
        );
        m_fields->m_popup = popup;
        popup->m_buttonMenu->setVisible(false);
        popup->show();
        CCArray* actions = CCArray::create();
        actions->addObject(CCDelayTime::create(2.f));
        actions->addObject(CCCallFunc::create(this, callfunc_selector(MyEditorPauseLayer::showPopupButton)));
        popup->runAction(CCSequence::create(actions));
    }

    void onCopyLevelString(CCObject* sender) {
        std::string levelCopyType = Mod::get()->getSettingValue<std::string>("level-copy-type");
        std::string levelString = m_editorLayer->m_level->m_levelString;
        levelString = Utils::processLevelString(levelString);

        if (levelCopyType == "Encoded") {
            clipboard::write(levelString);
            Notification* n = Notification::create("Encoded level copied successfully!", NotificationIcon::Success);
            n->show();
        } else if (levelCopyType == "Decoded") {
            std::string decrypted = ZipUtils::decompressString(levelString, false, 1);
            clipboard::write(decrypted);
            Notification* n = Notification::create("Decoded level copied successfully!", NotificationIcon::Success);
            n->show();
        } else {
            Notification* n = Notification::create("Unknown level type", NotificationIcon::Error);
            n->show();
        }
    }

    void showPopupButton() {
        m_fields->m_popup->m_buttonMenu->setVisible(true);
    }
};

class $modify(MyLevelAreaInnerLayer, LevelAreaInnerLayer) {
    bool init(bool returning) {
        if (!LevelAreaInnerLayer::init(returning)) return false;

        bool copyMainLevels = Mod::get()->getSettingValue<bool>("copy-main-levels");

        if (copyMainLevels) {
            auto mainNode = this->getChildByID("main-node");
            if (mainNode) {
                auto menu = mainNode->getChildByID("main-menu");
                if (menu) {
                    std::vector<CCNode*> copyButtons {};
                    for (CCNode* button : menu->getChildrenExt()) {
                        if (button->getTag() > 0) {
                            CCSprite* buttonSprite = CircleButtonSprite::create(
                                CCLabelBMFont::create("Copy\nLevel", "bigFont.fnt", 0.f, CCTextAlignment::kCCTextAlignmentCenter),
                                CircleBaseColor::Green,
                                CircleBaseSize::Small
                            );
                            CCMenuItemSpriteExtra* copyButton = CCMenuItemSpriteExtra::create(
                                buttonSprite,
                                this,
                                menu_selector(MyLevelAreaInnerLayer::onCopyLevel)
                            );

                            copyButton->setPositionX(button->getPositionX());
                            copyButton->setPositionY(button->getPositionY() - 45.f);
                            copyButton->setUserObject("level-id-to-copy"_spr, CCInteger::create(button->getTag()));
                            // copyButton->setTag(button->getTag());
                            // menu->addChild(copyButton);
                            copyButtons.push_back(copyButton);
                        }
                    }
                    for (CCNode* copyButtonToAdd : copyButtons) {
                        menu->addChild(copyButtonToAdd);
                    }
                } else {
                    log::debug("no menu");
                }
            } else {
                log::debug("no main node");
            }
        }

        return true;
    }

    void onCopyLevel(CCObject* sender) {
        if (!sender || !typeinfo_cast<CCInteger*>(static_cast<CCNode*>(sender)->getUserObject("level-id-to-copy"_spr))) return;
        GameLevelManager* glm = GameLevelManager::sharedState();
        GameManager* gm = GameManager::sharedState();
        LocalLevelManager* llm = LocalLevelManager::sharedState();
        CCDirector* ccd = CCDirector::sharedDirector();

        if (ccd->getIsTransitioning()) return;
        this->setKeypadEnabled(false);

        // log::debug("Level string for \"{}\": {}", m_level->m_levelName, m_level->m_levelString);
        auto levelID = static_cast<CCInteger*>(static_cast<CCNode*>(sender)->getUserObject("level-id-to-copy"_spr))->getValue();
        
        gm->m_sceneEnum = 2;
        GJGameLevel* level = glm->createNewLevel();
        level->copyLevelInfo(GameLevelManager::sharedState()->getMainLevel(levelID, true));
        level->m_levelType = GJLevelType::Editor;
        std::string levelString = llm->getMainLevelString(levelID);
        levelString = Utils::processLevelString(levelString);
        level->m_levelString = levelString;

        CCScene* scene = EditLevelLayer::scene(level);
        CCTransitionFade* transition = CCTransitionFade::create(0.5f, scene);
        ccd->replaceScene(transition);
    }
};
