![State](https://img.shields.io/badge/State-In%20progress-brightgreen?style=flat-square)
![lang](https://img.shields.io/badge/Language-C%2B%2B%2FHLSL-blue?style=flat-square)
![lib](https://img.shields.io/badge/Type-Desktop-blue?style=flat-square)
![lib](https://img.shields.io/badge/Lib-DirectX12%2FDirectXTex-%236658A6?style=flat-square)
# DirectX12Leaning
※これは僕の学習用に開発している描画エンジン/ライブラリであり、実務用途での使用を想定していません。

## 注意
***このプロジェクトをクローン/ダウンロードして実行ができない場合このような原因が挙げられます***  

・**プロジェクトのフィルタにコードが含まれていない**  
　　　→VS2019以降で開きプロジェクトを右クリック -> 追加から既存の項目で含まれていないコードを追加してください    
   
・**DirectX12がPCに対応していない**  
　　　→対応したOS/ハードウェアで実行してください  

・**まゆC#がバグを直していない**   
　　　→学習用のプロジェクトなので以下の対応をお願いいたします  
　　　　　・ご自身で直していただく  
　　　　　・プルリクエストで提案する  
　　　　　・気長に待つ  

## サンプルコード  
・以下のコードは実行すると自機と敵機が描画されます。  
・WASD / Arrowキーどちらかで操作することで自機が移動します。  
・敵機に触れることで敵機は自動的にランダムな場所へワープします。  
・エスケープキーを押す / xボタンを押すことで終了します。
```cpp
//API(Win)
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

//shader(HLSL)
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

//STL
#include <ctime>
#include <vector>
#include <string>
#include <assert.h>

//Utility
#include <dinput.h>
#include "Input.h"
#include "Win32.h"
#include "tempUtility.h"
#include "DirectX12.h"
#include "PlayerOP.h"
#include "Draw2D.h"
#include "Draw3D.h"

int WINAPI WinMain(_In_ HINSTANCE hinstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) 
{
	srand(time(nullptr));
	const int window_width = 1920;
	const int window_height = 1080;

	//Initialize
	Win32		win32(L"Test", window_width, window_height);
	Input		*input = new Input(win32.GetWindowClass(), win32.GetHandleWindow());
	DirectX12	dx12(win32.GetHandleWindow(), window_width, window_height);

	dx12.Initialize_components();
	ID3D12Device *dev = dx12.GetDevice();
	ID3D12GraphicsCommandList *cmdList = dx12.GetCommandList();

	//PLayer
	Draw3D drawPlayer(3, 5, D3D12_FILL_MODE_SOLID, dev, cmdList, window_width, window_height);
	PlayerOP player(0, 0, 0, 5, input);

	//DrawObject
	Draw3D enemyObject( 3, 5, D3D12_FILL_MODE_SOLID, dev, cmdList, window_width, window_height);
	DirectX::XMFLOAT4 enemyColor = dx12.GetColor(255, 0, 0, 255);
	Position3D enemyPos = {-20, 0, 0};

	while (true)
	{
		input->Update();
		dx12.ClearDrawScreen(dx12.GetColor(100, 200, 255, 255));

		player.Update();

		if (player.GetCollition(enemyPos, 5)) {
			enemyPos.x = rand() % 100 - 50;
			enemyPos.y = rand() % 50 - 25;
		}

		drawPlayer.execute(dx12.GetColor(255, 255, 255, 255), player.GetPlayerPositionMatrix());
		enemyObject.execute(enemyColor, DirectX::XMMatrixTranslation(enemyPos.x, enemyPos.y, enemyPos.z));

		dx12.ScreenFlip();
		if (!win32.ProcessMessage()) { break; }
		if (input->GetKeyDown(DIK_ESCAPE)) { break; }
	}
	return 0;
}
```  
***©2021 わんころメソッド();  
©2021 wnkr();*** 
