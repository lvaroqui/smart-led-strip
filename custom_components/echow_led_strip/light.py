"""Platform for light integration."""
from __future__ import annotations

import logging
from typing import Any
from homeassistant.helpers.entity import DeviceInfo
import colorsys

import voluptuous as vol

# Import the device class from the component that you want to support
import homeassistant.helpers.config_validation as cv
from homeassistant.components.light import (
    ATTR_BRIGHTNESS,
    ATTR_EFFECT,
    ATTR_HS_COLOR,
    ATTR_RGBW_COLOR,
    ATTR_TRANSITION,
    ATTR_WHITE,
    COLOR_MODE_HS,
    COLOR_MODE_WHITE,
    PLATFORM_SCHEMA,
    COLOR_MODE_RGBW,
    SUPPORT_EFFECT,
    SUPPORT_TRANSITION,
    LightEntity,
)
from homeassistant.const import CONF_HOST, CONF_NAME
from homeassistant.core import HomeAssistant
from homeassistant.config_entries import ConfigEntry
from homeassistant.helpers.entity_platform import AddEntitiesCallback
from homeassistant.helpers.typing import ConfigType, DiscoveryInfoType

import requests
import json

from .const import DOMAIN

_LOGGER = logging.getLogger(__name__)

# Validation of the user's configuration
PLATFORM_SCHEMA = PLATFORM_SCHEMA.extend(
    {
        vol.Required(CONF_HOST): cv.string,
    }
)


class CannotConnect(Exception):
    """Error to indicate we cannot connect."""


class SmartLedStrip:
    def __init__(self, host: str) -> None:
        self._host = host

    def info(self) -> str:
        try:
            r = requests.post(
                f"http://{self._host}/command",
                json.dumps(
                    {
                        "method": "get_info",
                    }
                ),
            )
            return json.loads(r.content.decode("utf-8"))
        except requests.exceptions.RequestException as error:
            _LOGGER.error(error)
            raise CannotConnect from error

    def set_rgbw(self, values: tuple[int, int, int, int], time: int = 1):
        try:
            requests.post(
                f"http://{self._host}/command",
                json.dumps(
                    {
                        "method": "set_rgbw",
                        "param": {
                            "r": values[0],
                            "g": values[1],
                            "b": values[2],
                            "w": values[3],
                            "time": time * 1000,
                        },
                    }
                ),
            )
        except requests.exceptions.RequestException as error:
            _LOGGER.error(error)
            raise CannotConnect from error

    def set_power(self, value: bool):
        try:
            requests.post(
                f"http://{self._host}/command",
                json.dumps(
                    {
                        "method": "set_power",
                        "param": {"value": value},
                    }
                ),
            )
        except requests.exceptions.RequestException as error:
            _LOGGER.error(error)
            raise CannotConnect from error

    def status(self):
        try:
            r = requests.post(
                f"http://{self._host}/command",
                json.dumps(
                    {
                        "method": "get_status",
                    }
                ),
            )
            status = json.loads(r.content.decode("utf-8"))
            return {
                "power": status["power"],
                "rgbw": (status["r"], status["g"], status["b"], status["w"]),
            }
        except requests.exceptions.RequestException as error:
            _LOGGER.error(error)
            raise CannotConnect from error


async def async_setup_entry(
    hass: HomeAssistant,
    config_entry: ConfigEntry,
    async_add_entities,
    discovery_info: DiscoveryInfoType | None = None,
) -> None:
    """Set up the Awesome Light platform."""
    # Assign configuration variables.
    # The configuration check takes care they are present.
    host = config_entry.data[CONF_HOST]
    name = config_entry.data[CONF_NAME]

    # Setup connection with devices/cloud
    led_strip = SmartLedStrip(host)

    # Verify that passed in configuration works
    # if not hub.is_valid_login():
    #     _LOGGER.error("Could not connect to AwesomeLight hub")
    #     return

    # Add devices
    async_add_entities([LedStrip(led_strip, name)])


class LedStrip(LightEntity):
    """Representation of an Awesome Light."""

    def __init__(self, light: SmartLedStrip, name: str) -> None:
        """Initialize an AwesomeLight."""
        self._light = light
        self._name = name
        self._brightness = 0
        self._hs_color = (0, 0)
        self._is_on = False
        self._availbable = True
        self._color_mode = COLOR_MODE_WHITE

    @property
    def device_info(self) -> DeviceInfo:
        return {
            "identifiers": {
                # Serial numbers are unique identifiers within a specific domain
                (DOMAIN, self.unique_id)
            },
            "name": self.name,
            "manufacturer": "Echow",
            "model": "Smart Led Strip",
            "sw_version": "1",
        }

    @property
    def name(self) -> str:
        """Return the display name of this light."""
        return self._name

    @property
    def unique_id(self) -> str:
        return self._light._host

    @property
    def is_on(self) -> bool | None:
        """Return true if light is on."""
        return self._is_on

    @property
    def supported_color_modes(self) -> set[str]:
        return {COLOR_MODE_HS, COLOR_MODE_WHITE}

    @property
    def supported_features(self) -> int:
        return SUPPORT_EFFECT | SUPPORT_TRANSITION

    @property
    def brightness(self) -> int:
        return self._brightness

    @property
    def color_mode(self) -> str:
        return self._color_mode

    @property
    def hs_color(self) -> tuple[float, float] | None:
        return self._hs_color

    @property
    def effect_list(self) -> list[str]:
        return ["test"]

    @property
    def effect(self) -> str | None:
        return None

    @property
    def should_poll(self) -> bool:
        return True

    def turn_on(self, **kwargs: Any) -> None:
        """Instruct the light to turn on."""
        white = kwargs.get(ATTR_WHITE)
        brightness = kwargs.get(ATTR_BRIGHTNESS)
        hs_color = kwargs.get(ATTR_HS_COLOR)
        effect = kwargs.get(ATTR_EFFECT)
        time = kwargs.get(ATTR_TRANSITION)

        if not white and not brightness and not hs_color and not effect:
            self._light.set_power(True)
            return

        if white:
            _color_mode = COLOR_MODE_WHITE
            self._brightness = white
        else:
            _color_mode = COLOR_MODE_HS

        if brightness:
            self._brightness = brightness

        if hs_color:
            self._hs_color = hs_color

        rgb = (0, 0, 0)
        white = 0
        if _color_mode == COLOR_MODE_HS:
            rgb = colorsys.hsv_to_rgb(
                self._hs_color[0] / 360, self._hs_color[1] / 100, self._brightness / 255
            )
        elif _color_mode == COLOR_MODE_WHITE:
            white = self._brightness / 255

        if not time:
            time = 1

        self._light.set_rgbw(rgb + (white,), time)

        self._light.set_power(True)

    def turn_off(self, **kwargs: Any) -> None:
        """Instruct the light to turn off."""
        self._light.set_power(False)

    def update(self) -> None:
        """Fetch new state data for this light.
        This is the only method that should fetch new data for Home Assistant.
        """

        status = self._light.status()

        power = status["power"]
        red, green, blue, white = status["rgbw"]

        self._is_on = power

        if white != 0:
            self._color_mode = COLOR_MODE_WHITE
            self._brightness = white * 255
        else:
            self._color_mode = COLOR_MODE_HS
            hue, sat, value = colorsys.rgb_to_hsv(red, green, blue)
            self._brightness = value * 255
            self._hs_color = (hue * 360, sat * 100)
