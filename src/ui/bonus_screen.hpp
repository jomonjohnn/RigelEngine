/* Copyright (C) 2016, Nikolai Wuttke. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <engine/timing.hpp>
#include <ui/menu_element_renderer.hpp>
#include <game_mode.hpp>

#include <functional>
#include <set>


namespace rigel { namespace ui {

class BonusScreen {
public:
  using BonusNumber = int;

  BonusScreen(
    GameMode::Context context,
    const std::set<BonusNumber>& achievedBonuses,
    int scoreBeforeAddingBonuses);

  void updateAndRender(engine::TimeDelta dt);

private:
  struct Event {
    engine::TimeDelta mTime;
    std::function<void()> mAction;
  };

  engine::TimeDelta setupBonusSummationSequence(
    const std::set<BonusNumber>& achievedBonuses);
  engine::TimeDelta setupNoBonusSequence();
  void updateSequence(engine::TimeDelta timeDelta);

private:
  int mScore;
  std::string mRunningText;

  engine::TimeDelta mElapsedTime = 0.0;
  std::vector<Event> mEvents;
  std::size_t mNextEvent = 0;
  bool mIsDone = false;

  SDL_Renderer* mpRenderer;
  IGameServiceProvider* mpServiceProvider;
  sdl_utils::OwningTexture mBackgroundTexture;
  ui::MenuElementRenderer mTextRenderer;
};

}}