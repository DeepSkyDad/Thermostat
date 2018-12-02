$(document).ready(function () {
	var showLoader = function () {
		$('.custom-loader').show();
	};

	var hideLoader = function () {
		$('.custom-loader').hide();
	};

	var checkStatus = function (callback) {
		$.ajax({
			url: "/api/status",
			error: function () {
				alert("Error");
				hideLoader();
				//alert("Error");
			},
			success: function (data) {
				hideLoader();
				populateStatus(data);
				if (callback)
					callback();
			},
			timeout: 10000 // sets timeout to 3 seconds
		});
	}

	var populateStatus = function(data) {
		$('#temperature').html(data.temperature.toFixed(2).replace('.', ',') + ' C');

		$('input[name="burner"]').parent().removeClass('active');
		$('input[name="heating_pump"]').parent().removeClass('active');
		$('input[name="water_pump"]').parent().removeClass('active');
		
		$('input[name="burner"][value=' + (data['burner'])  + ']').parent().addClass('active');
		$('input[name="heating_pump"][value=' + (data['heating_pump'])  + ']').parent().addClass('active');
		$('input[name="water_pump"][value=' + (data['water_pump'])  + ']').parent().addClass('active');
	};

	$("#burner :input").change(function() {
		save("burner", parseInt(this.value));
	});

	$("#heating_pump :input").change(function() {
		save("heating_pump", parseInt(this.value));
	});

	$("#water_pump :input").change(function() {
		save("water_pump", parseInt(this.value));
	});
	
	/*$("#tempUpBtn").click(function() {
		var newVal = parseFloat($("#tempInput").val()) + 0.5;
		
		$("#tempInput").val(newVal);
	});
	
	$("#tempDownBtn").click(function() {
		var newVal = parseFloat($("#tempInput").val()) - 0.5;
		
		$("#tempInput").val(newVal);
	});*/
	
	var save = function(key, value) {
		var data = {};
		data[key] = value;
		showLoader();
		$.ajax({
			type: "POST",
			url: "/api/save",
			data: JSON.stringify(data),
			contentType: 'application/json',
			error: function () {
				alert("error");
				hideLoader();
			},
			success: function (data) {
				hideLoader();
			}
		});
	};
	
	showLoader();
	checkStatus();

	setInterval(function() {
		checkStatus();
	}, 5000);
});
