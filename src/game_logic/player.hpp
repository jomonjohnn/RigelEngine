/* Copyright (C) 2018, Nikolai Wuttke. All rights reserved.
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

#include "base/warnings.hpp"
#include "data/game_session_data.hpp"
#include "engine/base_components.hpp"
#include "engine/movement.hpp"
#include "game_logic/input.hpp"
#include "game_logic/player/components.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

#include <bitset>
#include <cstdint>
#include <variant>

namespace rigel {
  struct IGameServiceProvider;

  namespace data {
    class PlayerModel;

    namespace map { class Map; }
  }

  namespace engine { class CollisionChecker; }

  namespace game_logic { struct IEntityFactory; }
}


namespace rigel { namespace game_logic {

enum class WeaponStance {
  Regular,
  RegularCrouched,
  Upwards,
  Downwards
};


struct OnGround {};

struct Jumping {
  struct FromLadder {}; // tag

  Jumping() = default;
  explicit Jumping(const FromLadder&)
    : mJumpedFromLadder(true)
  {
  }

  std::uint16_t mFramesElapsed = 0;
  bool mJumpedFromLadder = false;
};

struct Falling {
  int mFramesElapsed = 0;
};

struct PushedByFan {};

struct RecoveringFromLanding {};

struct ClimbingLadder {};

struct OnPipe {};

struct Interacting {
  explicit Interacting(const int duration)
    : mDuration(duration)
  {
  }

  int mDuration = 0;
  int mFramesElapsed = 0;
};


struct Incapacitated {
  int mVisibleFramesRemaining;
};


namespace death_animation {

struct FlyingUp {
  FlyingUp() = default;

  int mFramesElapsed = 0;
};

struct FallingDown {};

struct Exploding {
  int mFramesElapsed = 0;
};

struct Finished {};

}


using Dieing = std::variant<
  death_animation::FlyingUp,
  death_animation::FallingDown,
  death_animation::Exploding,
  death_animation::Finished>;


using PlayerState = std::variant<
  OnGround,
  Jumping,
  Falling,
  PushedByFan,
  RecoveringFromLanding,
  ClimbingLadder,
  OnPipe,
  Interacting,
  Incapacitated,
  Dieing>;


// The enum's values are chosen to match the corresponding animation frames.
// For animated states (like walking), the first frame of the cycle/sequence is
// used.
enum class VisualState {
  Standing = 0,
  Walking = 1,
  LookingUp = 16,
  Crouching = 17,
  HangingFromPipe = 20,
  MovingOnPipe = 21,
  AimingDownOnPipe = 25,
  PullingLegsUpOnPipe = 28,
  CoilingForJumpOrLanding = 5,
  Jumping = 6,
  DoingSalto = 9,
  Falling = 7,
  FallingFullSpeed = 8,
  Interacting = 33,
  ClimbingLadder = 35,
  UsingJetpack = 37,
  Dieing = 29,
  Dead = 32
};


struct AnimationConfig;


enum class SpiderClingPosition {
  Head = 0,
  Weapon = 1,
  Back = 2
};


class Player : public entityx::Receiver<Player> {
public:
  Player(
    entityx::Entity entity,
    data::Difficulty difficulty,
    data::PlayerModel* pModel,
    IGameServiceProvider* pServiceProvider,
    const engine::CollisionChecker* pCollisionChecker,
    const data::map::Map* pMap,
    IEntityFactory* pEntityFactory,
    entityx::EventManager* pEvents);
  Player(const Player&) = delete;
  Player(Player&&) = default;

  Player& operator=(const Player&) = delete;
  Player& operator=(Player&&) = default;

  void update(const PlayerInput& inputs);

  void takeDamage(int amount);
  void die();

  void incapacitate(int framesToKeepVisible = 0);
  void setFree();

  void doInteractionAnimation();

  void resetAfterDeath(entityx::Entity newEntity);
  void resetAfterRespawn();

  bool isInRegularState() const;

  bool canTakeDamage() const;
  bool isInMercyFrames() const;
  bool isCloaked() const;
  bool isDead() const;
  bool isIncapacitated() const;
  bool isLookingUp() const;
  bool isCrouching() const;
  engine::components::Orientation orientation() const;
  engine::components::BoundingBox worldSpaceHitBox() const;
  engine::components::BoundingBox worldSpaceCollisionBox() const;
  const base::Vector& position() const;
  int animationFrame() const;

  // TODO: Explain what this is
  base::Vector orientedPosition() const;

  template<typename StateT>
  bool stateIs() const {
    return std::holds_alternative<StateT>(mState);
  }

  base::Vector& position();

  data::PlayerModel& model() {
    return *mpPlayerModel;
  }

  void receive(const events::ElevatorAttachmentChanged& event);

  bool hasSpiderAt(const SpiderClingPosition position) const {
    return mAttachedSpiders.test(static_cast<size_t>(position));
  }

  void attachSpider(const SpiderClingPosition position) {
    mAttachedSpiders.set(static_cast<size_t>(position));
  }

  void detachSpider(const SpiderClingPosition position) {
    mAttachedSpiders.reset(static_cast<size_t>(position));
  }

  void beginBeingPushedByFan();
  void endBeingPushedByFan();

private:
  struct VerticalMovementResult {
    engine::MovementResult mMoveResult = engine::MovementResult::Failed;
    bool mAttachedToClimbable = false;
  };

  void updateTemporaryItemExpiration();
  void updateAnimation();
  void updateMovement(const base::Vector& movementVector, const Button& jumpButton);
  void updateShooting(const Button& fireButton);
  void updateLadderAttachment(const base::Vector& movementVector);
  bool updateElevatorMovement(int movement);
  void updateHorizontalMovementInAir(const base::Vector& movementVector);
  void updateJumpMovement(
    Jumping& state,
    const base::Vector& movementVector,
    bool jumpPressed);
  void updateDeathAnimation();
  void updateIncapacitatedState(Incapacitated& state);

  VerticalMovementResult moveVerticallyInAir(int amount);

  void updateAnimationLoop(const AnimationConfig& config);
  void resetAnimation();
  void updateMercyFramesAnimation();
  void updateCloakedAppearance();
  void updateCollisionBox();
  void updateHitBox();

  void fireShot();

  void setVisualState(VisualState visualState);
  void jump();
  void jumpFromLadder(const base::Vector& movementVector);
  void startFalling();
  void startFallingDelayed();
  void landOnGround(bool needRecoveryFrame);
  void switchOrientation();

  PlayerState mState;
  entityx::Entity mEntity;
  entityx::Entity mAttachedElevator;
  data::PlayerModel* mpPlayerModel;
  IGameServiceProvider* mpServiceProvider;
  const engine::CollisionChecker* mpCollisionChecker;
  const data::map::Map* mpMap;
  IEntityFactory* mpEntityFactory;
  entityx::EventManager* mpEvents;
  engine::components::BoundingBox mHitBox;
  WeaponStance mStance = WeaponStance::Regular;
  VisualState mVisualState = VisualState::Standing;
  int mMercyFramesPerHit;
  int mMercyFramesRemaining;
  int mFramesElapsedHavingRapidFire = 0;
  int mFramesElapsedHavingCloak = 0;
  std::bitset<3> mAttachedSpiders;
  bool mRapidFiredLastFrame = false;
  bool mIsOddFrame = false;
  bool mRecoilAnimationActive = false;
  bool mIsRidingElevator = false;
};

}}
