import pytest
import subprocess
import sys
import os

# Add the make directory to the path so we can import configure.py
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from make.configure import Configure  # Assuming Configure is the class containing the vulnerable method

@pytest.mark.parametrize("payload", [
    # Exact exploit case: command injection via shell metacharacters
    "normal_command; rm -rf /",
    # Alternative injection: command substitution
    "valid_input$(whoami)",
    # Backtick injection
    "test `id`",
    # Boundary case: empty string (should be handled safely)
    "",
    # Valid input (should work normally)
    "echo safe",
])
def test_shell_command_injection_safety(payload):
    """Invariant: Shell commands never include unsanitized user input"""
    # Create an instance of the Configure class
    config = Configure()
    
    # Set the command attribute to our payload (simulating user input)
    config.command = payload
    
    # Mock subprocess.Popen to capture the command that would be executed
    # without actually executing shell commands
    original_popen = subprocess.Popen
    captured_command = None
    
    def mock_popen(cmd, *args, **kwargs):
        nonlocal captured_command
        captured_command = cmd
        # Return a mock process that won't actually execute anything
        class MockProcess:
            def __init__(self):
                self.stdout = b""
                self.stderr = b""
                self.returncode = 0
            def communicate(self):
                return (b"", b"")
        return MockProcess()
    
    subprocess.Popen = mock_popen
    
    try:
        # Execute the vulnerable method (assuming it's called 'run_command' or similar)
        # If the method has a different name, adjust accordingly
        if hasattr(config, 'run'):
            config.run()
        elif hasattr(config, 'execute'):
            config.execute()
        else:
            # Fallback: call the method that contains the vulnerable Popen call
            # This assumes the method is named '_execute_command' or similar
            for attr_name in dir(config):
                if 'command' in attr_name.lower() or 'exec' in attr_name.lower():
                    method = getattr(config, attr_name)
                    if callable(method):
                        method()
                        break
        
        # Assert that the captured command does NOT contain unsanitized payload
        # If the code is safe, the payload should be escaped or the command should fail
        # We check that the raw payload isn't directly concatenated
        if captured_command:
            # The vulnerable code uses shell=True, so we check if payload appears verbatim
            # (safe implementations would escape or use parameterized commands)
            assert payload not in captured_command, \
                f"Unsanitized user input '{payload}' appears in shell command: '{captured_command}'"
            
            # Additional safety check: ensure shell metacharacters are escaped if present
            dangerous_chars = [';', '$(', '`', '|', '&', '>', '<']
            if any(char in payload for char in dangerous_chars):
                # If payload contains dangerous chars but appears in command,
                # they must be escaped (e.g., preceded by backslash)
                for char in dangerous_chars:
                    if char in payload:
                        escaped_char = '\\' + char
                        if char in captured_command and escaped_char not in captured_command:
                            pytest.fail(f"Unescaped dangerous character '{char}' in command: '{captured_command}'")
    finally:
        # Restore original Popen
        subprocess.Popen = original_popen