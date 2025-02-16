// Fill out your copyright notice in the Description page of Project Settings.


#include "PBDRopeActor.h"
#include "GameFramework/Character.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

//! コンストラクタ
//*****************************************************************************
APBDRopeActor::APBDRopeActor()
{
	PrimaryActorTick.bCanEverTick = true;
}

//! 開始
//*****************************************************************************
void APBDRopeActor::BeginPlay()
{
	Super::BeginPlay();
	
    // 紐の初期化
    // MEMO: UEだと2つの減衰係数と重力を大きな値にすると、それっぽくなった
	for (int i = 0; i < pointNum_; i++) {
        massPoints_.Add(MassPoint(mass_, GetActorLocation() + FVector(i, 0, 0), i == 0));
	}
    for (int i = 0; i < pointNum_ - 1; i++) {
        if (isPBD_) {
            constraints_.Add(MakeUnique<DistanceConstraint_PBD>(length_, stiffness_, &massPoints_[i], &massPoints_[i + 1]));
        }
        else {
            constraints_.Add(MakeUnique<DistanceConstraint_XPBD>(length_, compliance_, &massPoints_[i], &massPoints_[i + 1]));
        }
    }
}

//! 更新
//*****************************************************************************
void APBDRopeActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    // シミュレーション
    float dt = DeltaTime / step_;
    for (int i = 0; i < step_; i++) {
        Simulate(dt, i);
    }
    //Simulate(dt, step_);

    // デバッグ描画
    for (auto& p : massPoints_) {
        UKismetSystemLibrary::DrawDebugSphere(GetWorld(), p.position_, 2.0f, 12, p.isKinematic_ ? FColor::Red : FColor::Blue);
    }
    for (auto& c : constraints_) {
        UKismetSystemLibrary::DrawDebugLine(GetWorld(), c->a_->position_, c->b_->position_, FColor::Green);
    }

    // 無理やり終点をPCに設定してみる
    if (isConnectPlayer_) {
        ACharacter* pc = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
        if (pc && massPoints_.Num() > 0) {
            massPoints_[pointNum_ - 1].setPosition(pc->GetActorLocation());
            massPoints_[pointNum_ - 1].isKinematic_ = true;
        }
    }
}

//! シミュレーション
//*****************************************************************************
void APBDRopeActor::Simulate(float dt, int iter)
{
    // 座標更新
    for(auto& p : massPoints_){
		p.updatePosition(dt);
	}

    // 拘束計算の前処理
    for (auto& c : constraints_) {
        c->initLambda();
    }

    // 拘束計算
    for (int i = 0; i < iter; i++) {
        for (auto& c : constraints_) {
            c->solvePosition(dt);
        }
    }

    // 速度を更新
    for (auto& p : massPoints_) {
        p.updateVelocity(dt, -gravity_);
    }

    // 速度の減衰各種
    for (auto& p : massPoints_) {
        p.solveVelocity(dt, globalDampingCoeff_);
    }
    for (auto& c : constraints_) {
        c->solveVelocity(dt, edgeDampingCoeff_);
    }
}

//! クリップボードにパラメーターをコピー
//*****************************************************************************
void APBDRopeActor::CopyRopeParametersToClipboard() const
{
    FString ClipboardText = FString::Printf(TEXT("step_[%d], globalDampingCoeff_[%.2f], edgeDampingCoeff_[%.2f], pointNum_[%d], gravity_[%.2f], length_[%.2f], stiffness_[%.2f], compliance_[%.2f], mass_[%.2f]"),
        step_, globalDampingCoeff_, edgeDampingCoeff_, pointNum_, gravity_, length_, stiffness_, compliance_, mass_);
    FPlatformApplicationMisc::ClipboardCopy(*ClipboardText);

    // 拘束計算の反復回数 step_
    // 速度の減衰係数 globalDampingCoeff_
    // エッジの速度の減衰係数 edgeDampingCoeff_
    // 質点(制御点)の数 pointNum_
    // 重力 gravity_
    // 拘束の長さ length_
    // 拘束の固さ0から1の範囲、1で最も固くなる stiffness_
    // コンプライアンス値(剛性の逆数)、0に近いほど固い compliance_
    // 質点の質量 mass_
}
