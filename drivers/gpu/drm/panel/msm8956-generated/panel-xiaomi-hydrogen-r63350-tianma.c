// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2026 FIXME

// Generated with linux-mdss-dsi-panel-driver-generator from vendor device tree:
//   Copyright (c) 2013, The Linux Foundation. All rights reserved. (FIXME)

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>
#include <video/mipi_display.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/drm_probe_helper.h>

struct tianma_r63350 {
    struct drm_panel panel;
    struct mipi_dsi_device *dsi;
    struct gpio_desc *reset_gpio;
    struct gpio_desc *enable_gpio;
    struct regulator *vsp;
    struct regulator *vsn;
};

static inline struct tianma_r63350 *to_tianma_r63350(struct drm_panel *panel)
{
    return container_of_const(panel, struct tianma_r63350, panel);
}

static void tianma_r63350_reset(struct tianma_r63350 *ctx)
{
    gpiod_set_value_cansleep(ctx->reset_gpio, 1);
    usleep_range(5000, 6000);
    gpiod_set_value_cansleep(ctx->reset_gpio, 0);
    usleep_range(10000, 11000);
}

static int tianma_r63350_on(struct tianma_r63350 *ctx)
{
    struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

    mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb0, 0x00);
    mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd6, 0x01);
    mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd3,
                     0x1b, 0x33, 0x99, 0xbb, 0xb3, 0x33,
                     0x33, 0x33, 0x11, 0x00, 0x01, 0x00,
                     0x00, 0xd8, 0xa0, 0x05, 0x3f, 0x3f,
                     0x33, 0x33, 0x72, 0x12, 0x8a, 0x57,
                     0x3d, 0xbc);
    mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc7,
                     0x00, 0x12, 0x1a, 0x25, 0x33, 0x42,
                     0x4c, 0x5c, 0x42, 0x4a, 0x55, 0x5f,
                     0x69, 0x6f, 0x75, 0x00, 0x12, 0x1a,
                     0x25, 0x33, 0x42, 0x4c, 0x5c, 0x42,
                     0x4a, 0x55, 0x5f, 0x69, 0x6f, 0x75);
    mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc8,
                     0x01, 0x00, 0xfe, 0x00, 0xfe, 0xc8,
                     0x00, 0x00, 0x02, 0x00, 0x00, 0xfc,
                     0x00, 0x04, 0xfe, 0x04, 0x0d, 0xed,
                     0x00);
    mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb0, 0x03);
    mipi_dsi_dcs_set_display_brightness_multi(&dsi_ctx, 0x00ff);
    mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_WRITE_CONTROL_DISPLAY,
                     0x24);
    mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_WRITE_POWER_SAVE, 0x00);

    /* Restored original vendor sequence */
    mipi_dsi_dcs_set_display_on_multi(&dsi_ctx);
    mipi_dsi_msleep(&dsi_ctx, 20);
    mipi_dsi_dcs_exit_sleep_mode_multi(&dsi_ctx);
    mipi_dsi_msleep(&dsi_ctx, 120);

    return dsi_ctx.accum_err;
}

static int tianma_r63350_off(struct tianma_r63350 *ctx)
{
    struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

    mipi_dsi_dcs_set_display_off_multi(&dsi_ctx);
    mipi_dsi_msleep(&dsi_ctx, 20);
    mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb0, 0x00);
    mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd3,
                     0x13, 0x33, 0x99, 0xb3, 0xb3, 0x33,
                     0x33, 0x33, 0x11, 0x00, 0x01, 0x00,
                     0x00, 0xd8, 0xa0, 0x05, 0x3f, 0x3f,
                     0x33, 0x33, 0x72, 0x12, 0x8a, 0x57,
                     0x3d, 0xbc);
    mipi_dsi_msleep(&dsi_ctx, 50);
    mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb0, 0x03);
    mipi_dsi_dcs_enter_sleep_mode_multi(&dsi_ctx);
    mipi_dsi_msleep(&dsi_ctx, 50);

    return dsi_ctx.accum_err;
}

static int tianma_r63350_prepare(struct drm_panel *panel)
{
    struct tianma_r63350 *ctx = to_tianma_r63350(panel);
    struct device *dev = &ctx->dsi->dev;
    int ret;

    /* 1. Enable display panel power supplies (LAB/IBB) */
    ret = regulator_enable(ctx->vsp);
    if (ret) {
        dev_err(dev, "Failed to enable vsp: %d\n", ret);
        goto err_reset;
    }

    ret = regulator_enable(ctx->vsn);
    if (ret) {
        dev_err(dev, "Failed to enable vsn: %d\n", ret);
        goto err_disable_vsp;
    }
    msleep(10);

    /* 2. Reset the panel */
    tianma_r63350_reset(ctx);

    /* 3. Send init commands */
    ret = tianma_r63350_on(ctx);
    if (ret < 0) {
        dev_err(dev, "Failed to initialize panel: %d\n", ret);
        goto err_disable_vsn;
    }

    /* 4. Enable backlight power (GPIO 66) AFTER init to prevent glitch */
    gpiod_set_value_cansleep(ctx->enable_gpio, 1);

    return 0;

err_disable_vsn:
    regulator_disable(ctx->vsn);
err_disable_vsp:
    regulator_disable(ctx->vsp);
err_reset:
    gpiod_set_value_cansleep(ctx->reset_gpio, 1);
    return ret;
}

static int tianma_r63350_unprepare(struct drm_panel *panel)
{
    struct tianma_r63350 *ctx = to_tianma_r63350(panel);
    struct device *dev = &ctx->dsi->dev;
    int ret;

    /* 1. Disable backlight power first to prevent glitch */
    gpiod_set_value_cansleep(ctx->enable_gpio, 0);

    ret = tianma_r63350_off(ctx);
    if (ret < 0)
        dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

    gpiod_set_value_cansleep(ctx->reset_gpio, 1);
    
    regulator_disable(ctx->vsn);
    regulator_disable(ctx->vsp);
    
    return 0;
}

static const struct drm_display_mode tianma_r63350_mode = {
    .clock = (1080 + 80 + 10 + 40) * (1920 + 4 + 2 + 4) * 60 / 1000,
    .hdisplay = 1080,
    .hsync_start = 1080 + 80,
    .hsync_end = 1080 + 80 + 10,
    .htotal = 1080 + 80 + 10 + 40,
    .vdisplay = 1920,
    .vsync_start = 1920 + 4,
    .vsync_end = 1920 + 4 + 2,
    .vtotal = 1920 + 4 + 2 + 4,
    .width_mm = 80,
    .height_mm = 142,
    .type = DRM_MODE_TYPE_DRIVER,
};

static int tianma_r63350_get_modes(struct drm_panel *panel,
                   struct drm_connector *connector)
{
    return drm_connector_helper_get_modes_fixed(connector, &tianma_r63350_mode);
}

static const struct drm_panel_funcs tianma_r63350_panel_funcs = {
    .prepare = tianma_r63350_prepare,
    .unprepare = tianma_r63350_unprepare,
    .get_modes = tianma_r63350_get_modes,
};

static int tianma_r63350_probe(struct mipi_dsi_device *dsi)
{
    struct device *dev = &dsi->dev;
    struct tianma_r63350 *ctx;
    int ret;

    ctx = devm_drm_panel_alloc(dev, struct tianma_r63350, panel,
                   &tianma_r63350_panel_funcs,
                   DRM_MODE_CONNECTOR_DSI);
    if (IS_ERR(ctx))
        return PTR_ERR(ctx);

    ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
    if (IS_ERR(ctx->reset_gpio))
        return dev_err_probe(dev, PTR_ERR(ctx->reset_gpio),
                     "Failed to get reset-gpios\n");

    ctx->enable_gpio = devm_gpiod_get(dev, "enable", GPIOD_OUT_LOW);
    if (IS_ERR(ctx->enable_gpio))
        return dev_err_probe(dev, PTR_ERR(ctx->enable_gpio),
                     "Failed to get enable-gpios\n");

    ctx->vsp = devm_regulator_get(dev, "vsp");
    if (IS_ERR(ctx->vsp))
        return dev_err_probe(dev, PTR_ERR(ctx->vsp), "Failed to get vsp\n");

    ctx->vsn = devm_regulator_get(dev, "vsn");
    if (IS_ERR(ctx->vsn))
        return dev_err_probe(dev, PTR_ERR(ctx->vsn), "Failed to get vsn\n");

    ctx->dsi = dsi;
    mipi_dsi_set_drvdata(dsi, ctx);

    dsi->lanes = 4;
    dsi->format = MIPI_DSI_FMT_RGB888;
    dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
              MIPI_DSI_CLOCK_NON_CONTINUOUS;

    ctx->panel.prepare_prev_first = true;

    ret = drm_panel_of_backlight(&ctx->panel);
    if (ret)
        return dev_err_probe(dev, ret, "Failed to get backlight\n");

    drm_panel_add(&ctx->panel);

    ret = mipi_dsi_attach(dsi);
    if (ret < 0) {
        drm_panel_remove(&ctx->panel);
        return dev_err_probe(dev, ret, "Failed to attach to DSI host\n");
    }

    return 0;
}

static void tianma_r63350_remove(struct mipi_dsi_device *dsi)
{
    struct tianma_r63350 *ctx = mipi_dsi_get_drvdata(dsi);
    int ret;

    ret = mipi_dsi_detach(dsi);
    if (ret < 0)
        dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

    drm_panel_remove(&ctx->panel);
}

static const struct of_device_id tianma_r63350_of_match[] = {
    { .compatible = "tianma,r63350" },
    { .compatible = "xiaomi,hydrogen-panel" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, tianma_r63350_of_match);

static struct mipi_dsi_driver tianma_r63350_driver = {
    .probe = tianma_r63350_probe,
    .remove = tianma_r63350_remove,
    .driver = {
        .name = "panel-tianma-r63350",
        .of_match_table = tianma_r63350_of_match,
    },
};
module_mipi_dsi_driver(tianma_r63350_driver);

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me.com>"); // FIXME
MODULE_DESCRIPTION("DRM driver for tianma r63350 1080p video mode dsi panel");
MODULE_LICENSE("GPL");
