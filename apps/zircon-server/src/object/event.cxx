#include "object/event.h"

zx_status_t create_eventpair(ZxEventPair *&ep0, ZxEventPair *&ep1)
{
    ep0 = ep1 = NULL;

    ep0 = allocate_object<ZxEventPair>();
    if (ep0 == NULL) {
        return ZX_ERR_NO_MEMORY;
    }
    ep1 = allocate_object<ZxEventPair>();
    if (ep1 == NULL) {
        free_object(ep0);
        ep0 = NULL;
        return ZX_ERR_NO_MEMORY;
    }

    /* Set peers */
    ep0->peer_ = ep1;
    ep1->peer_ = ep0;
    return ZX_OK;
}

void ZxEventPair::destroy()
{
    /* Let peer know we are being destroyed */
    ZxEventPair *other = peer_;
    if (other != NULL) {
        other->peer_ = NULL;
        other->update_state(0u, ZX_EPAIR_PEER_CLOSED);
        // TODO invalidate cookie
    }
}

zx_status_t ZxEventPair::user_signal(uint32_t clear_mask, uint32_t set_mask, bool peer)
{
    if (peer) {
        return ZX_ERR_NOT_SUPPORTED;
    }

    constexpr uint32_t kUserSignalMask = ZX_EPAIR_SIGNALED | ZX_USER_SIGNAL_ALL;
    if ((set_mask & ~kUserSignalMask) || (clear_mask & ~kUserSignalMask)) {
        return ZX_ERR_INVALID_ARGS;
    }

    if (!peer) {
        update_state(clear_mask, set_mask);
        return ZX_OK;
    }

    if (peer_ == NULL) {
        return ZX_ERR_PEER_CLOSED;
    }

    peer_->update_state(clear_mask, set_mask);
    return ZX_OK;
}
