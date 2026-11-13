async function ledOn() {
	await fetch('/api/led/on');
}

async function setColor() {
	const color = document.getElementById("colorPicker").value;

	await fetch('/api/color/' + color.substring(1));
}
