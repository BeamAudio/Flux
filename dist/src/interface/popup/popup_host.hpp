#ifndef POPUP_HOST_HPP
#define POPUP_HOST_HPP

#include "interface/core/component.hpp"
#include <memory>

namespace Beam {

class PopupHost {
public:
    virtual ~PopupHost() = default;
    virtual void showPopup(std::shared_ptr<Component> popup) = 0;
    virtual void closePopup() = 0;
};

} // namespace Beam

#endif // POPUP_HOST_HPP
