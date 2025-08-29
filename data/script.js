document.addEventListener('DOMContentLoaded', () => {
    const turnOnLightBtn = document.getElementById('turnOnLightBtn');
    const turnOffLightBtn = document.getElementById('turnOffLightBtn');
    const lightStatusSpan = document.getElementById('lightStatus');
    const onAngleInput = document.getElementById('onAngleInput');
    const setOnAngleBtn = document.getElementById('setOnAngleBtn');
    const offAngleInput = document.getElementById('offAngleInput');
    const setOffAngleBtn = document.getElementById('setOffAngleBtn');
    const autoResetToggle = document.getElementById('autoResetToggle');
    const autoResetAngleInput = document.getElementById('autoResetAngleInput');
    const setAutoResetAngleBtn = document.getElementById('setAutoResetAngleBtn');
    const ipAddressSpan = document.getElementById('ipAddress');

    // 初始化页面状态
    function updateStatus() {
        fetch('/status')
            .then(response => response.json())
            .then(data => {
                lightStatusSpan.textContent = data.isLightOn ? '开' : '关';
                autoResetToggle.checked = data.isAutoResetEnabled;
                autoResetAngleInput.value = data.autoResetAngle;
                onAngleInput.value = data.onAngle;
                offAngleInput.value = data.offAngle;
            })
            .catch(error => console.error('获取状态失败:', error));

        fetch('/ip')
            .then(response => response.text())
            .then(ip => {
                ipAddressSpan.textContent = ip;
            })
            .catch(error => console.error('获取IP地址失败:', error));
    }

    // 开灯
    turnOnLightBtn.addEventListener('click', () => {
        const angle = parseInt(onAngleInput.value);
        if (isNaN(angle) || angle < 0 || angle > 180) {
            alert('请输入0到180之间的有效开灯角度！');
            return;
        }
        fetch(`/turnLightOn?angle=${angle}`)
            .then(response => response.text())
            .then(status => {
                lightStatusSpan.textContent = (status === 'ON') ? '开' : '关';
            })
            .catch(error => console.error('开灯失败:', error));
    });

    // 关灯
    turnOffLightBtn.addEventListener('click', () => {
        const angle = parseInt(offAngleInput.value);
        if (isNaN(angle) || angle < 0 || angle > 180) {
            alert('请输入0到180之间的有效关灯角度！');
            return;
        }
        fetch(`/turnLightOff?angle=${angle}`)
            .then(response => response.text())
            .then(status => {
                lightStatusSpan.textContent = (status === 'OFF') ? '关' : '开';
            })
            .catch(error => console.error('关灯失败:', error));
    });

    // 设置开灯角度
    setOnAngleBtn.addEventListener('click', () => {
        const angle = parseInt(onAngleInput.value);
        if (isNaN(angle) || angle < 0 || angle > 180) {
            alert('请输入0到180之间的有效开灯角度！');
            return;
        }
        fetch(`/setOnAngle?angle=${angle}`)
            .then(response => {
                if (response.ok) {
                    console.log('开灯角度设置成功');
                } else {
                    alert('设置开灯角度失败，请检查输入。');
                }
            })
            .catch(error => console.error('设置开灯角度失败:', error));
    });

    // 设置关灯角度
    setOffAngleBtn.addEventListener('click', () => {
        const angle = parseInt(offAngleInput.value);
        if (isNaN(angle) || angle < 0 || angle > 180) {
            alert('请输入0到180之间的有效关灯角度！');
            return;
        }
        fetch(`/setOffAngle?angle=${angle}`)
            .then(response => {
                if (response.ok) {
                    console.log('关灯角度设置成功');
                } else {
                    alert('设置关灯角度失败，请检查输入。');
                }
            })
            .catch(error => console.error('设置关灯角度失败:', error));
    });

    // 切换自动复位功能
    autoResetToggle.addEventListener('change', () => {
        const enable = autoResetToggle.checked;
        fetch(`/toggleAutoReset?enable=${enable}`)
            .then(response => {
                if (response.ok) {
                    console.log('自动复位功能已' + (enable ? '启用' : '禁用'));
                } else {
                    alert('切换自动复位功能失败。');
                }
            })
            .catch(error => console.error('切换自动复位功能失败:', error));
    });

    // 设置自动复位角度
    setAutoResetAngleBtn.addEventListener('click', () => {
        const angle = parseInt(autoResetAngleInput.value);
        if (isNaN(angle) || angle < 0 || angle > 180) {
            alert('请输入0到180之间的有效复位角度！');
            return;
        }
        fetch(`/setAutoResetAngle?angle=${angle}`)
            .then(response => {
                if (response.ok) {
                    console.log('自动复位角度设置成功');
                } else {
                    alert('设置自动复位角度失败，请检查输入。');
                }
            })
            .catch(error => console.error('设置自动复位角度失败:', error));
    });

    // 页面加载时更新一次状态
    updateStatus();
    // 每5秒更新一次IP地址，确保显示最新IP
    setInterval(updateStatus, 5000);
});
