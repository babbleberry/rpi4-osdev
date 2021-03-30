//
//  ViewController.swift
//

import UIKit
import CoreBluetooth

class ViewController: UIViewController,CBPeripheralManagerDelegate {
    @IBOutlet weak var messageLabel: UILabel!
    @IBOutlet weak var fireButton: UIButton!
    
    private var service: CBUUID!
    private var peripheralManager : CBPeripheralManager!
    let myCharacteristic1 = CBMutableCharacteristic(type: CBUUID(string: "EC0E"), properties: [.notify, .write, .read], value: nil, permissions: [.readable, .writeable])
        
    override func viewDidLoad() {
        super.viewDidLoad()
        peripheralManager = CBPeripheralManager(delegate: self, queue: nil)
    }
    
    @IBAction func onFireUp() {
        let fireup : [UInt8] = [2, 1]
        
        messageLabel.text = "Fired"
        
        var fireupData = Data()
        fireupData.append(contentsOf: fireup)
        
        peripheralManager.updateValue(fireupData, for: myCharacteristic1, onSubscribedCentrals: nil)
    }
    
    @IBAction func onFireDown() {
        let firedown : [UInt8] = [1, 1]
        
        messageLabel.text = "Firing"
        
        var firedownData = Data()
        firedownData.append(contentsOf: firedown)
        
        peripheralManager.updateValue(firedownData, for: myCharacteristic1, onSubscribedCentrals: nil)
    }
    
    @IBAction func handlePan(_ gesture: UIPanGestureRecognizer) {
        let screenSize: CGRect = UIScreen.main.bounds
        let tap = gesture.location(in: view)

        let maxx = screenSize.width
        let maxy = screenSize.height
            
        let realx = UInt16(((tap.x / maxx) * 320).rounded())
        let realy = UInt16(((tap.y / maxy) * 200).rounded())
        
        let coords : [UInt8] = [UInt8(realx & 0x00ff), UInt8(realx >> 8 & 0x00ff), UInt8(realy & 0x00ff), UInt8(realy >> 8 & 0x00ff)]
        
        var coordData = Data()
        coordData.append(contentsOf: coords)
        
        peripheralManager.updateValue(coordData, for: myCharacteristic1, onSubscribedCentrals: nil)
    }
    
    func peripheralManagerDidUpdateState(_ peripheral: CBPeripheralManager) {
        switch peripheral.state {
        case .unknown:
            print("Bluetooth Device is UNKNOWN")
        case .unsupported:
            print("Bluetooth Device is UNSUPPORTED")
        case .unauthorized:
            print("Bluetooth Device is UNAUTHORIZED")
        case .resetting:
            print("Bluetooth Device is RESETTING")
        case .poweredOff:
            print("Bluetooth Device is POWERED OFF")
        case .poweredOn:
            print("Bluetooth Device is POWERED ON")
            addServices()
        @unknown default:
            fatalError()
        }
    }

    func addServices() {
        service = CBUUID(string: "EC00")
        let myService = CBMutableService(type: service, primary: true)
        myService.characteristics = [myCharacteristic1]
        peripheralManager.add(myService)
        startAdvertising()
    }
    
    func startAdvertising() {
        messageLabel.text = "Waiting for subscriber"
        peripheralManager.startAdvertising([CBAdvertisementDataLocalNameKey : "echo", CBAdvertisementDataServiceUUIDsKey : [service]])
    }
    
    func peripheralManager(_ peripheral: CBPeripheralManager, central: CBCentral, didSubscribeTo characteristic: CBCharacteristic) {
        messageLabel.text = "Sub \(central.identifier.uuidString)"
    }
    
    func peripheralManager(_ peripheral: CBPeripheralManager, central: CBCentral, didUnsubscribeFrom characteristic: CBCharacteristic) {
        messageLabel.text = "Unsub \(central.identifier.uuidString)"
    }
}
