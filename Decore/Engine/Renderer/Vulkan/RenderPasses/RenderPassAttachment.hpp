#pragma once

namespace Engine::Renderer {
    class RenderPassAttachment {
    public:
        /*
        TODO: Investigate if I need to support
            VkResolveModeFlagBits    resolveMode;
            VkImageView              resolveImageView;
            VkImageLayout            resolveImageLayout;
        */

        RenderPassAttachment() = default;
        RenderPassAttachment(const VkImage image, const VkImageView imageView, const VkImageLayout imageLayout, const VkFormat imageFormat, const VkAttachmentLoadOp loadOp, const VkAttachmentStoreOp storeOp, VkClearValue clearValue);

        VkRenderingAttachmentInfo GetInfo() const;

        VkImage GetImage() const { return m_Image; }
        VkImageView GetImageView() const { return m_ImageView; }
        VkImageLayout GetImageLayout() const { return m_ImageLayout; }
        VkFormat GetImageFormat() const { return m_ImageFormat; }
    private:
        VkImage m_Image{};
        VkFormat m_ImageFormat{};
        VkImageView m_ImageView{};
        VkImageLayout m_ImageLayout{};
        VkAttachmentLoadOp m_LoadOp{};
        VkAttachmentStoreOp m_StoreOp{};
        VkClearValue m_ClearValue{};
    };
}