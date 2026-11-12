// Standalone layout test. Does not create a window or link the renderer.
#include "../SimpleGame/GameUI.h"
#include <cassert>
#include <cmath>

int main()
{
    for (const auto size : {std::array<int,2>{640,360}, {1280,720}, {1920,1080}, {2560,1440}})
    {
        const auto controls = GameUiControls(size[0], size[1]);
        for (size_t i=0; i<controls.size(); ++i)
        {
            const auto &a = controls[i];
            assert(a.x-a.radius>=0 && a.x+a.radius<size[0]);
            assert(a.y-a.radius>=0 && a.y+a.radius<size[1]);
            assert(HitTestGameUi(static_cast<int>(a.x),static_cast<int>(a.y),size[0],size[1])==a.command);
            for (size_t j=i+1; j<controls.size(); ++j)
            {
                const auto &b = controls[j];
                assert(std::hypot(a.x-b.x,a.y-b.y)>a.radius+b.radius);
            }
        }
        assert(HitTestGameUi(size[0]/2,size[1]/2,size[0],size[1])==GameCommand::None);
        const float s = GameUiScale(size[0],size[1]);
        const auto inventory = InventoryPanel(size[0],size[1]);
        assert(inventory.x >= 0 && inventory.y >= 0);
        assert(inventory.x + inventory.width <= size[0] && inventory.y + inventory.height <= size[1]);
        const auto buttons = InventoryButtons(size[0],size[1]);
        for (size_t i=0; i<buttons.size(); ++i)
        {
            const auto &button = buttons[i];
            assert(button.x >= inventory.x && button.y >= inventory.y);
            assert(button.x + button.width <= inventory.x + inventory.width);
            assert(button.y + button.height <= inventory.y + inventory.height);
            assert(HitTestGameUi(static_cast<int>(button.x+button.width/2),
                static_cast<int>(button.y+button.height/2),size[0],size[1],true)==button.command);
            for (size_t j=i+1;j<buttons.size();++j)
                assert(button.x+button.width<=buttons[j].x || buttons[j].x+buttons[j].width<=button.x ||
                    button.y+button.height<=buttons[j].y || buttons[j].y+buttons[j].height<=button.y);
        }
        assert(GameUiPanelContains(static_cast<int>(50*s),static_cast<int>(50*s),size[0],size[1]));
        assert(!GameUiPanelContains(size[0]/2,size[1]/2,size[0],size[1]));
    }
}
