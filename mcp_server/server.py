"""
MCP server version of agent.py
Implements the same tools (calculator, LED control, time, multiply)
"""
import json
import datetime
from typing import Optional
from mcp.server.fastmcp import FastMCP

import os
from simpleio import usb_led

# --- Import hardware control helpers (if available) ---
# try:
    # import os
    # from .simpleio import usb_led
# except ImportError:
    # def usb_led(blue, red, green, yellow):
        # print(f"Simulated LED -> blue={blue}, red={red}, green={green}, yellow={yellow}")
    
# --- Initialize MCP server ---
mcp = FastMCP("general-assistant-mcp")

# --- Tool: calculator ---
@mcp.tool()
def calculator(expression: str) -> str:
    """
    Evaluate a mathematical expression.
    Example: calculator("1+1")
    """
    try:
        return str(eval(expression))
    except Exception as e:
        return f"Error: {e}"

# --- Tool: LED control ---
@mcp.tool()
def led_control(
    blue: Optional[int] = None,
    red: Optional[int] = None,
    green: Optional[int] = None,
    yellow: Optional[int] = None,
) -> str:
    """
    Turn LEDs on/off.
    Example: led_control(red=1)
    """
    print(f"\nuser control led -> blue:{blue}, red:{red}, green:{green}, yellow:{yellow}")
    usb_led(blue or 0, red or 0, green or 0, yellow or 0)
    led = {"blue": blue, "red": red, "green": green, "yellow": yellow}
    return json.dumps(led)

# --- Tool: Get current time ---
@mcp.tool()
def get_current_time() -> str:
    """
    Get current date and time.
    Example: get_current_time()
    """
    today = datetime.datetime.today()
    date_time_data = {
        "day_now": today.strftime("%Y-%m-%d"),
        "time_now": today.strftime("%H:%M"),
    }
    return json.dumps(date_time_data)

# --- Tool: Multiply numbers ---
@mcp.tool()
def multiply(a: int, b: int) -> int:
    """
    Multiply two numbers.
    Example: multiply(3, 4)
    """
    return a * b

# --- Resource: Info ---
@mcp.resource("info://agent")
def agent_info() -> str:
    """Provide general information about this agent MCP server."""
    return (
        "General Assistant MCP Server — can calculate, control LEDs, "
        "get current time, and multiply numbers."
    )

# --- Main entry ---
if __name__ == "__main__":
    mcp.run()
