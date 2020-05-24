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
				//alert("Error");
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
		$('#tempHum').html(data.temperature.toFixed(2).replace('.', ',') + ' C / ' + data.humidity.toFixed(2).replace('.', ',') + ' %');

		/*$('input[name="burner"]').parent().removeClass('active');
		$('input[name="heating_pump"]').parent().removeClass('active');
		$('input[name="water_pump"]').parent().removeClass('active');
		
		$('input[name="burner"][value=' + (data['burner'])  + ']').parent().addClass('active');
		$('input[name="heating_pump"][value=' + (data['heating_pump'])  + ']').parent().addClass('active');
		$('input[name="water_pump"][value=' + (data['water_pump'])  + ']').parent().addClass('active');*/

		$('select[name="burner"]').val(data['burner']);
		$('select[name="heating_pump"]').val(data['heating_pump']);
		$('select[name="water_pump"]').val(data['water_pump']);

		if(data['burner'] == 2) {
			$("#heatingWrapper").hide();
			$("#waterWrapper").hide();
		} else {
			$("#heatingWrapper").show();
			$("#waterWrapper").show();
		}

		$('#relayIndicatorBurner').removeClass('relay-indicator-on');
		$('#relayIndicatorHeatingPump').removeClass('relay-indicator-on');
		$('#relayIndicatorWaterPump').removeClass('relay-indicator-on');
		
		if(data["relay_burner"]) $('#relayIndicatorBurner').addClass('relay-indicator-on');
		if(data["relay_heating_pump"]) $('#relayIndicatorHeatingPump').addClass('relay-indicator-on');
		if(data["relay_water_pump"]) $('#relayIndicatorWaterPump').addClass('relay-indicator-on');
		
				
	};

	$("#burner").change($.proxy(function() {
		var burner = parseInt($("#burner").val());
		
		if(burner == 2) {
			$("#heatingWrapper").hide();
			$("#waterWrapper").hide();
		} else {
			$("#heatingWrapper").show();
			$("#waterWrapper").show();
		}

		saveFn("burner", burner, false);
	}, this));

	$("#heating_pump").change($.proxy(function() {
		saveFn("heating_pump", parseInt($("#heating_pump").val()), false);
	}, this));

	$("#water_pump").change($.proxy(function() {
		saveFn("water_pump", parseInt($("#water_pump").val()), false);
	}, this));
	
	/*$("#tempUpBtn").click(function() {
		var newVal = parseFloat($("#tempInput").val()) + 0.5;
		
		$("#tempInput").val(newVal);
	});
	
	$("#tempDownBtn").click(function() {
		var newVal = parseFloat($("#tempInput").val()) - 0.5;
		
		$("#tempInput").val(newVal);
	});*/
	
	var saveFn = function(key, value, supressRetry) {
		var data = {};
		data[key] = value;
		showLoader();
		$.ajax({
			type: "POST",
			url: "/api/save",
			data: JSON.stringify(data),
			contentType: 'application/json',
			error: function () {
				if(!supressRetry)
					saveFn(key, value, true);
				hideLoader();
			},
			success: function (data) {
				hideLoader();
				populateStatus(data);
			}
		});
	};
	
	showLoader();
	checkStatus();

	setInterval(function() {
		checkStatus();
	}, 15000);
});
