$(document).ready(function () {
	var showLoader = function () {
		$('.custom-loader').show();
	};

	var hideLoader = function () {
		$('.custom-loader').hide();
	};

	var checkStatus = function (callback) {
		showLoader();
		$.ajax({
			url: "/api/status",
			error: function () {
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
		$('input[name="burner"][value=' + (data['burner'])  + ']').parent().addClass('active');
		$('input[name="burner"][value=' + (data['burner'] == 1 ? 0 : 1)  + ']').parent().removeClass('active');
	};

	$("#burner :input").change(function() {
		save("burner", parseInt(this.value));
	});
	
	$("#tempUpBtn").click(function() {
		var newVal = parseFloat($("#tempInput").val()) + 0.5;
		
		$("#tempInput").val(newVal);
	});
	
	$("#tempDownBtn").click(function() {
		var newVal = parseFloat($("#tempInput").val()) - 0.5;
		
		$("#tempInput").val(newVal);
	});
	
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
				hideLoader();
			},
			success: function (data) {
				hideLoader();
			}
		});
	};
	
	checkStatus();
});
